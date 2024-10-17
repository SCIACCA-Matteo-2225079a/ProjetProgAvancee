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
    cout << "Client " << id << " ne peut pas entrer, salon plein (balk)." << endl;
}

void wakeUpEmploye(int id){
    cout << "Client " << id << " réveille le barbier." << endl;
}

void sleepEmploye(){
    cout << "Le barbier s'endort car il n'y a plus de clients." << endl;

}

void employe() {
    while (true) {
        unique_lock<mutex> lock(mtx);

        // Attendre qu'il y ait des clients dans la file d'attente
        employe_cv.wait(lock, [] { return !waitingVoitures.empty(); });

        // Le barbier est réveillé et peut couper les cheveux
        currentVoitureId = waitingVoitures.front(); // Prendre le client en tête de la queue
        waitingVoitures.pop(); // Retirer le client de la file d'attente

        cout << "Le barbier dit au client " << currentVoitureId << " de s'installer pour la coupe." << endl;

        // Signaler au client qu'il peut se faire couper les cheveux
        voiture_cv.notify_one();

        cout << "Le barbier commence à couper les cheveux pour le client " << currentVoitureId << "." << endl;
        this_thread::sleep_for(chrono::seconds(2)); // Simuler le temps de coupe

        cout << "Le barbier a terminé de couper les cheveux pour le client " << currentVoitureId << "." << endl;

        // Indiquer que le client peut partir
        voitureDone = true;
        cout << "Le barbier dit au client " << currentVoitureId << " de partir." << endl;

        // Réinitialiser l'indicateur pour indiquer que la coupe est finie
        voitureDone = false;

        if (waitingVoitures.empty()) {
            employeSleeping = true;
            sleepEmploye();
        }

        // Signaler au client que la coupe est terminée
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

        cout << "Client " << id << " entre dans le salon." << endl;

        waitingVoitures.push(id); // Ajouter le client à la file

        // Réveiller le barbier
        if (employeSleeping) {
            wakeUpEmploye(id);
            employeSleeping = false;
            employe_cv.notify_one(); // Signaler le barbier qu'il y a un client
        }
    }

    // Attendre que le barbier lui dise de s'installer pour la coupe
    {
        unique_lock<mutex> lock(mtx);
        while (currentVoitureId != id) {
            voiture_cv.wait(lock); // Attendre que le barbier lui signale de s'installer
        }
    }

    // Attendre que le barbier termine la coupe
    {
        unique_lock<mutex> lock(mtx);
        while (!voitureDone) {
            voiture_cv.wait(lock); // Attendre que le barbier signale que la coupe est terminée
        }
    }

    servedVoitures.push_back(id);
    //cout << "Client " << id << " quitte le salon." << endl;
}

int main() {

    thread employeThread(employe);


    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(1, 3);

    int voitureId = 1;
    while (true) {
        for (int i = 0; i < 3; ++i) { // Simule 3 clients entrant presque en même temps
            thread voitureThread(voiture, voitureId);
            voitureThread.detach();
            voitureId++;
        }

        this_thread::sleep_for(chrono::seconds(dis(gen))); // Attendre un temps aléatoire avant d'ajouter le prochain groupe de clients
    }
    return 0;
}
