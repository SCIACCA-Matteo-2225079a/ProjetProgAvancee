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
condition_variable barber_cv;   // Pour signaler le barbier
condition_variable customer_cv;  // Pour signaler le client

// Indicateurs pour gérer l'état du barbier et des clients
bool barberSleeping = true; // État du barbier (dormant ou éveillé)
queue<int> waitingCustomers; // File d'attente pour les clients
bool customerDone = false;   // Indique si le client a terminé
int currentCustomerId = -1;  // Identifiant du client en cours
vector<int> servedCustomers;  // Vecteur pour garder une trace des clients déjà servis
const int maxCustomers = 6;   // Maximum de clients dans le barbershop

// Fonction représentant le barbier
void barber() {
    while (true) {
        unique_lock<mutex> lock(mtx);

        // Attendre qu'il y ait des clients dans la file d'attente
        barber_cv.wait(lock, [] { return !waitingCustomers.empty(); });

        // Le barbier est réveillé et peut couper les cheveux
        currentCustomerId = waitingCustomers.front(); // Prendre le client en tête de la queue
        waitingCustomers.pop(); // Retirer le client de la file d'attente

        // Le barbier commence à couper les cheveux
        cout << "Le barbier commence à couper les cheveux pour le client " << currentCustomerId << "." << endl;
        this_thread::sleep_for(chrono::seconds(2)); // Simuler le temps de coupe

        // Indiquer que la coupe est terminée
        cout << "Le barbier a terminé de couper les cheveux pour le client " << currentCustomerId << "." << endl;

        // Indiquer que le client peut partir
        customerDone = true; // Le client est maintenant prêt à partir
        cout << "Le barbier dit au client " << currentCustomerId << " de partir." << endl;

        // Réinitialiser l'indicateur pour indiquer que la coupe est finie
        customerDone = false;

        // Si la file d'attente est vide, le barbier peut dormir
        if (waitingCustomers.empty()) {
            barberSleeping = true;
            cout << "Le barbier s'endort car il n'y a plus de clients." << endl;
        }

        // Signaler au client que la coupe est terminée
        customer_cv.notify_all();
    }
}

// Fonction représentant un client
void customer(int id) {
    {
        lock_guard<mutex> lock(mtx);

        // Vérifier si le salon est plein
        if (waitingCustomers.size() >= maxCustomers) {
            cout << "Client " << id << " ne peut pas entrer, salon plein (balk)." << endl;
            return; // Salon plein, le client part
        }

        cout << "Client " << id << " entre dans le salon." << endl;

        // Ajouter le client à la file d'attente
        waitingCustomers.push(id); // Ajouter le client à la queue

        // Réveiller le barbier
        if (barberSleeping) {
            cout << "Client " << id << " réveille le barbier." << endl;
            barberSleeping = false; // Le barbier est réveillé
            barber_cv.notify_one(); // Signaler le barbier qu'il y a un client
        }
    }

    // Attendre que le barbier lui dise de s'installer pour la coupe
    {
        unique_lock<mutex> lock(mtx);
        // Attendre que le barbier lui signale de s'installer
        while (currentCustomerId != id) {
            customer_cv.wait(lock); // Attendre que le barbier lui signale de s'installer
        }
    }

    // Attendre que le barbier termine la coupe
    {
        unique_lock<mutex> lock(mtx);
        // Attendre que le barbier signale que la coupe est terminée
        while (!customerDone) {
            customer_cv.wait(lock); // Attendre que le barbier signale que la coupe est terminée
        }
    }

    // Ajouter le client à la liste des clients servis
    servedCustomers.push_back(id);
    cout << "Client " << id << " quitte le salon." << endl;
}

int main() {
    // Lancer le thread du barbier
    thread barberThread(barber);

    // Utiliser un générateur de nombres aléatoires pour les délais d'arrivée des clients
    random_device rd; // Source de random
    mt19937 gen(rd()); // Générateur
    uniform_int_distribution<> dis(1, 3); // Distribution pour des délais d'attente entre 1 et 3 secondes

    // Boucle infinie pour simuler l'arrivée de clients
    int customerId = 1; // Identifiant du premier client
    while (true) {
        // Créer plusieurs clients qui arrivent en même temps
        for (int i = 0; i < 3; ++i) { // Simule 3 clients entrant presque en même temps
            thread customerThread(customer, customerId);
            customerThread.detach(); // Détacher le thread du client
            customerId++; // Incrémenter l'identifiant du client
        }

        this_thread::sleep_for(chrono::seconds(dis(gen))); // Attendre un temps aléatoire avant d'ajouter le prochain groupe de clients
    }

    // Le code ci-dessous ne sera jamais atteint en raison de la boucle infinie.
    return 0;
}
