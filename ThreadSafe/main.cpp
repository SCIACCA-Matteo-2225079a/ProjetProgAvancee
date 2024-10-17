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

void balk(int id){
    cout << "Client " << id << " ne peut pas entrer, salon plein (balk)." << endl;
}

void wakeUpBarber(int id){
    cout << "Client " << id << " réveille le barbier." << endl;
}

void sleepBarber(){
    cout << "Le barbier s'endort car il n'y a plus de clients." << endl;

}

void barber() {
    while (true) {
        unique_lock<mutex> lock(mtx);

        // Attendre qu'il y ait des clients dans la file d'attente
        barber_cv.wait(lock, [] { return !waitingCustomers.empty(); });

        // Le barbier est réveillé et peut couper les cheveux
        currentCustomerId = waitingCustomers.front(); // Prendre le client en tête de la queue
        waitingCustomers.pop(); // Retirer le client de la file d'attente

        cout << "Le barbier dit au client " << currentCustomerId << " de s'installer pour la coupe." << endl;

        // Signaler au client qu'il peut se faire couper les cheveux
        customer_cv.notify_one();

        cout << "Le barbier commence à couper les cheveux pour le client " << currentCustomerId << "." << endl;
        this_thread::sleep_for(chrono::seconds(2)); // Simuler le temps de coupe

        cout << "Le barbier a terminé de couper les cheveux pour le client " << currentCustomerId << "." << endl;

        // Indiquer que le client peut partir
        customerDone = true;
        cout << "Le barbier dit au client " << currentCustomerId << " de partir." << endl;

        // Réinitialiser l'indicateur pour indiquer que la coupe est finie
        customerDone = false;

        if (waitingCustomers.empty()) {
            barberSleeping = true;
            sleepBarber();
        }

        // Signaler au client que la coupe est terminée
        customer_cv.notify_all();
    }
}

void customer(int id) {
    {
        lock_guard<mutex> lock(mtx);

        if (waitingCustomers.size() >= maxCustomers) {
            balk(id);
            return;
        }

        cout << "Client " << id << " entre dans le salon." << endl;

        waitingCustomers.push(id); // Ajouter le client à la file

        // Réveiller le barbier
        if (barberSleeping) {
            wakeUpBarber(id);
            barberSleeping = false;
            barber_cv.notify_one(); // Signaler le barbier qu'il y a un client
        }
    }

    // Attendre que le barbier lui dise de s'installer pour la coupe
    {
        unique_lock<mutex> lock(mtx);
        while (currentCustomerId != id) {
            customer_cv.wait(lock); // Attendre que le barbier lui signale de s'installer
        }
    }

    // Attendre que le barbier termine la coupe
    {
        unique_lock<mutex> lock(mtx);
        while (!customerDone) {
            customer_cv.wait(lock); // Attendre que le barbier signale que la coupe est terminée
        }
    }

    servedCustomers.push_back(id);
    //cout << "Client " << id << " quitte le salon." << endl;
}

int main() {

    thread barberThread(barber);


    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(1, 3);

    int customerId = 1;
    while (true) {
        for (int i = 0; i < 3; ++i) { // Simule 3 clients entrant presque en même temps
            thread customerThread(customer, customerId);
            customerThread.detach();
            customerId++;
        }

        this_thread::sleep_for(chrono::seconds(dis(gen))); // Attendre un temps aléatoire avant d'ajouter le prochain groupe de clients
    }
    return 0;
}
