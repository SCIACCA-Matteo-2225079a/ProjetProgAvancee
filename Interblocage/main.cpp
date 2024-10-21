#include <iostream>
#include <thread>
#include <mutex>
#include <string>
#include <vector>
#include <atomic>

using namespace std;


// variable globale
unsigned n (4);
unsigned voitures = 0;
mutex mut;
mutex employer;
mutex voiture;
mutex voitureFait;
mutex employerFait;

void getCommande(){
    cout << "La voiture récupère sa commande" << endl;
}

void faireCommande(){
    cout << "L'employer prépare la commande" << endl;
}

void Voiture (){
    mut.lock();
    if(voitures == n){
        mut.unlock();
        cout << "Je pars y'a trop de monde" << endl;
        return;
    }
    ++voitures;
    mut.unlock();

    cout << "La voiture attend dans la queue" << endl;
    voiture.lock();
    employer.lock();

    getCommande();

    voitureFait.lock();
    employerFait.lock();

    mut.lock();
        voitures -= 1;
    cout << "Voiture pars" << endl;
    mut.unlock();

}

void Employer(){

    if(voitures == 0){
        employer.lock();
        cout << "L'employer repars en cuisine" << endl;
    } else {
        voiture.unlock();
        employer.unlock();

        faireCommande();

        voitureFait.unlock();
        employerFait.unlock();
    }

}

int main()
{
    vector<thread> vTh;

    // Ajouter au tableau de threads 2000 nouveau thread avec la fct inc
    for(int j = 0; j < 10; ++j){
        vTh.push_back(thread(Voiture));
    }

    //Fermeture de tout les threads
    for(auto & th : vTh)
        th.join();

    thread t2(Employer);
    t2.join();
}
