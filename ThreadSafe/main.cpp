#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <random>
#include <queue>
#include <vector>
#include <algorithm>

using namespace std;

// Mutex et condition variables pour la synchronisation
mutex mtx;
condition_variable employe_cv;   // Pour signaler le barbier
condition_variable voiture_cv;  // Pour signaler le client

// Indicateurs pour gérer l'état du barbier et des clients
bool employeSleeping = true; // État du barbier (dormant ou éveillé)
queue<int> waitingVoitures; // File d'attente pour les clients
bool voitureDone = false;   // Indique si le client a terminé
int currentVoitureId = -1;  // Identifiant du client en cours
vector<int> servedVoitures;  // Vecteur pour garder une trace des clients déjà servis
const int maxVoitures = 6;   // Maximum de clients dans le employeshop

void balk(int id){
    cout << "La voiture " << id << " ne peut pas entrer dans le drive, la file est pleine (balk)." << endl;
}

void wakeUpEmploye(int id){
    cout << "La voiture " << id << " sonne à la borne." << endl;
}

void sleepEmploye(){
    cout << "L'employé retourne en cuisine car il n'y a plus de voitures." << endl;

}

void employe() {
    while (true) {
        unique_lock<mutex> lock(mtx);

        // Attendre qu'il y ait des clients dans la file d'attente
        employe_cv.wait(lock, [] { return !waitingVoitures.empty(); });

        // l'employé est de retour à la borne et peut donner la commande
        currentVoitureId = waitingVoitures.front(); // Servir la première voiture
        waitingVoitures.pop(); // Retirer la première voiture de la file d'attente

        cout << "L'employé dit à la voiture " << currentVoitureId << " de s'avancer pour récupérer la commande." << endl;

        // Signaler à la voiture qu'il peut récupérer sa commande
        voiture_cv.notify_one();

        cout << "L'employé tend la commande à la voiture " << currentVoitureId << "." << endl;
        this_thread::sleep_for(chrono::seconds(2)); // Simuler le temps de la commande

        cout << "L'employé a donné la commande à la voiture " << currentVoitureId << "." << endl;

        // Indique que la voiture peut partir
        voitureDone = true;
        cout << "L'employé dit à la voiture " << currentVoitureId << " de partir." << endl;

        // Réinitialiser l'indicateur pour indiquer que la commande est finie
        voitureDone = false;

        if (waitingVoitures.empty()) {
            employeSleeping = true;
            sleepEmploye();
        }

        // Signaler à la voiture que la commande est récupérée
        voiture_cv.notify_all();
    }
}

void voiture(int id) {
    {
        lock_guard<mutex> lock(mtx);

        if (waitingVoitures.size() >= maxVoitures) {
            balk(id);
            return;
        }

        cout << "Voiture" << id << " entre dans la file du drive." << endl;

        waitingVoitures.push(id); // Ajouter la voiture à la file

        // Rappeler l'employé à la borne
        if (employeSleeping) {
            wakeUpEmploye(id);
            employeSleeping = false;
            employe_cv.notify_one(); // Signaler à l'employé  qu'il y a une voiture
        }
    }

    // Attendre que l'employé lui dise de s'avancer
    {
        unique_lock<mutex> lock(mtx);
        while (currentVoitureId != id) {
            voiture_cv.wait(lock); // Attendre que l'employé lui signale d'avancer
        }
    }

    // Attendre que l'employé donne la commande
    {
        unique_lock<mutex> lock(mtx);
        while (!voitureDone) {
            voiture_cv.wait(lock); // Attendre que l'employé signale qui la donné la commande
        }
    }

    servedVoitures.push_back(id);
    //cout << "Voiture " << id << " quitte le drive." << endl;
}

int main() {

    thread employeThread(employe);


    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(1, 3);

    int voitureId = 1;
    while (true) {
        for (int i = 0; i < 3; ++i) { // Simule 3 voitures entrant presque en même temps
            thread voitureThread(voiture, voitureId);
            voitureThread.detach();
            voitureId++;
        }

        this_thread::sleep_for(chrono::seconds(dis(gen))); // Attendre un temps aléatoire avant d'ajouter le prochain groupe de voitures
    }
    return 0;
}
