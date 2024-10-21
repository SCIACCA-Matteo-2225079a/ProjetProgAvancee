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
condition_variable employe_cv;   // Pour signaler les employés
condition_variable voiture_cv;   // Pour signaler les voitures

// Indicateurs pour gérer l'état des employés et des clients
queue<int> waitingVoitures;  // File d'attente pour les voitures
vector<int> servedVoitures;  // Vecteur pour garder une trace des voitures déjà servies
const int maxVoitures = 6;   // Maximum de voitures dans le drive

// Nombre d'employés
const int numEmployes = 1;
bool employeSleeping[numEmployes];  // État des employés (dormant ou éveillé)

void balk(int id) {
    cout << "La voiture " << id << " ne peut pas entrer dans le drive, la file est pleine (balk)." << endl;
}

void wakeUpEmploye(int id, int employeId) {
    cout << "La voiture " << id << " appelle l'employé " << employeId << " qui était en cuisine." << endl;
}

void sleepEmploye(int employeId) {
    cout << "L'employé " << employeId << " retourne en cuisine car il n'y a plus de voitures." << endl;
}

void employe(int employeId) {
    while (true) {
        unique_lock<mutex> lock(mtx);

        // Attendre qu'il y ait des voitures dans la file d'attente
        employe_cv.wait(lock, [] { return !waitingVoitures.empty(); });

        // L'employé prend une voiture de la file s'il y en a
        if (!waitingVoitures.empty()) {
            int currentVoitureId = waitingVoitures.front(); // Servir la première voiture
            waitingVoitures.pop();  // Retirer la première voiture de la file d'attente

            cout << "L'employé " << employeId << " dit à la voiture " << currentVoitureId << " de s'avancer pour récupérer la commande." << endl;

            // Simuler la prise de commande
            voiture_cv.notify_one(); // Signaler à la voiture de s'avancer
            cout << "L'employé " << employeId << " tend la commande à la voiture " << currentVoitureId << "." << endl;
            this_thread::sleep_for(chrono::seconds(2)); // Simuler le temps de la commande

            cout << "L'employé " << employeId << " a donné la commande à la voiture " << currentVoitureId << "." << endl;
            cout << "L'employé " << employeId << " dit à la voiture " << currentVoitureId << " de partir." << endl;

            // Ajouter la voiture aux voitures servies
            servedVoitures.push_back(currentVoitureId);

            if (waitingVoitures.empty()) {
                employeSleeping[employeId] = true;  // L'employé retourne en cuisine
                sleepEmploye(employeId);
            }
        }
    }
}

void voiture(int id) {
    {
        lock_guard<mutex> lock(mtx);

        if (waitingVoitures.size() >= maxVoitures) {
            balk(id);
            return;
        }

        cout << "Voiture " << id << " entre dans la file du drive." << endl;

        waitingVoitures.push(id); // Ajouter la voiture à la file

        // Rappeler un employé s'il y a une voiture dans la file
        for (int i = 0; i < numEmployes; ++i) {
            if (employeSleeping[i]) {
                wakeUpEmploye(id, i);
                employeSleeping[i] = false;
                employe_cv.notify_all(); // Signaler aux employés qu'il y a des voitures
                break;
            }
        }
    }

    // Attendre que l'employé lui dise de s'avancer
    {
        unique_lock<mutex> lock(mtx);
        voiture_cv.wait(lock); // Attendre que l'employé lui signale d'avancer
    }

    // Voiture a été servie
}

int main() {
    // Initialiser les employés comme étant tous en cuisine
    fill(begin(employeSleeping), end(employeSleeping), true);

    // Créer et lancer des threads pour chaque employé
    vector<thread> employeThreads;
    for (int i = 0; i < numEmployes; ++i) {
        employeThreads.push_back(thread(employe, i));
    }

    // Générer des voitures
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

    // Rejoindre les threads des employés (ne sera pas atteint ici car la boucle est infinie)
    for (auto& employeThread : employeThreads) {
        employeThread.join();
    }

    return 0;
}
