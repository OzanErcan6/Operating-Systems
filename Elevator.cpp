//using threads and condition variables to simulate an elevator system


#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<semaphore.h>
#include <iostream>
#include <vector>
#include "monitor.h"
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>


using namespace std;

int currentFloor;
int numberOfFloors;
int numberOfPeople;
int travelTime=0;
int IDLETime=0;
int InOutTime=0;

int indirilen=0;


int weightCapacity;
int personCapacity;
int state=0; //0 idle 1 up 2 down

vector <int> unservedpeople={};  //id list of unserved persons initial {1,1,1,1,1,..}
vector <int> destinationById={};
vector <int> initialById={};
vector <int> weights={};
vector <int> orders={};
vector <int> priorities={};



class Elevator:public Monitor {

    Condition condition;

    int served;

    int readcount =0;

    int totalWeight;
    int totalPerson;

    vector <int> personsInElevator;
    vector <int> destinations;


public:
    Elevator(int _numOfPerson) : condition(this) {
        totalWeight=0;
        totalPerson=0;
        served=0;

    }

    void start_read()
    {
        __synchronized__ ;
        while (readcount) { // there is a writer
            condition.wait();
        }
        readcount++;

    }

    void finish_read()
    {
        __synchronized__ ;
        served++;
        readcount--;
        if (!readcount) // last reader going out
            condition.notify();

    }


    void start_el()
    {
        __synchronized__ ;

        if (served < numberOfPeople) {     //is all person threads ran
            while (served <= numberOfPeople) {
                condition.wait();
            }
        }


        while (readcount) { // there is a writer
            condition.wait();
        }
        readcount++;

    }

    void finish_el()
    {
        __synchronized__ ;

        readcount--;
        served=0;
        if (!readcount) // last reader going out
            condition.notify();

    }


    int getTotalWeight(){__synchronized__ ; return totalWeight;}
    int getTotalPerson(){__synchronized__ ; return totalPerson;}


    //asansörde yer varsa true döner
    bool canGoIn(int id) {
        __synchronized__ ;
        if(weightCapacity >= (totalWeight+weights[id]))
            if(personCapacity >= totalPerson)
                return true;
        return false;
    }



    bool isInList(int a){
        for (int i = 0; i < destinations.size() ; ++i) {
            if( a == destinations[i])
                return true;
        }
        return false;
    }

    void setDestination(int a)
    {
        __synchronized__ ;
        if(!isInList(a))
            destinations.push_back(a);
    }





    //eğer current floor dest listte varsa remove
    void updateDestList(){
        destinations.erase(remove(destinations.begin(), destinations.end(), currentFloor), destinations.end());
    }



    void printDestList()
    {
        __synchronized__ ;
        if(destinations.empty()){
            cout <<")"<< endl;
            return;
        }
        for (int i = 0; i < destinations.size() ; ++i) {
            cout << destinations[i] ;
            if(i < destinations.size()-1)
                cout << "," ;
        }
        cout <<")"<< endl;



    }


    int getDest(){
        __synchronized__ ;
        if(destinations.empty()) return -1;
        return destinations[0];
    }

    // destinationa gelindi listeden ilk elementi sil
    int newDest(){
        __synchronized__ ;
        if(destinations.empty()) return -1;
        //return destinations[1];

        destinations.erase (destinations.begin());
        return destinations[0];
    }


    bool isInElevator(int id){
        __synchronized__ ;
        for(int i = 0 ; i<personsInElevator.size() ; i++){
            if(personsInElevator[i]==id) return true;
        }
        return false;
    }
    void removeFromElevator(int id){
        __synchronized__ ;
        for(int i = 0 ; i<personsInElevator.size() ; i++)
            if(personsInElevator[i]==id){
                personsInElevator[i]=-1;
                totalWeight = totalWeight- weights[id];
                totalPerson--;
            }
    }

    void insertToElevator(int id){
        __synchronized__ ;
        personsInElevator.push_back(id);
        totalWeight = totalWeight + weights[id];
        totalPerson++;
    }

    //served persons are 0
    void personServed(int id){
        __synchronized__ ;
        unservedpeople[id]=0;
    }


};


struct PParam {
    Elevator *bs;
    int bid;
    bool gaveOrder; // asansöre orderı verdiyse true
    int Priority;
};

void *rider(void *p)
{
    PParam *bp = (PParam *) p;
    Elevator *monitor = bp->bs;
    int id = bp->bid;

    while(indirilen!=numberOfPeople){
            monitor->start_read();

            if(!orders[id] && unservedpeople[id]){

                monitor->setDestination(initialById[id]);
                orders[id]=1;

                if(bp->Priority==2)
                    cout << "Person (" << id << ",lp," <<  initialById[id] << ","<< destinationById[id] << "," << weights[id] << ") made a request"<< endl;
                if(bp->Priority==1)
                    cout << "Person (" << id << ",hp," <<  initialById[id] << ","<< destinationById[id] << "," << weights[id] << ") made a request"<< endl;


            }


            monitor->finish_read();
        }

    if(currentFloor==destinationById[id] &&  !monitor->isInElevator(id) && unservedpeople[id]){
        orders[id]=0;

        cout << "tekrar order vermek istiyorum ...................."<< endl;
    }



    // eğer asansör bu kata geldiyse ve binemediyse order vermeye devam etsin


    return 0;
}

void *elevator_controller(void *p)
{

    PParam *bp = (PParam *) p;
    Elevator *monitor = bp->bs;

    int dest;
    int key=0;

    while(indirilen!=numberOfPeople){
        monitor->start_el();
        usleep( travelTime);
        dest = monitor->getDest();

        if(dest == -1 ){
            state =0;
        }
        if(dest > currentFloor){
            state=1;
            currentFloor++;
        }
        if(dest < currentFloor ){
            state=2;
            currentFloor--;
        }

        if(state==0){
            cout << "Elevator (Idle," << monitor->getTotalWeight() <<","<< monitor->getTotalPerson() <<","<<  currentFloor << "->" ;// << monitor->printDestList() << endl;
            monitor->printDestList();
        }
        if(state==1){
            cout << "Elevator (Moving-up," << monitor->getTotalWeight() <<","<< monitor->getTotalPerson() <<","<<  currentFloor << "->" ;// << monitor->printDestList() << endl;
            monitor->printDestList();
        }
        if(state==2){
            cout << "Elevator (Moving-down," << monitor->getTotalWeight() <<","<< monitor->getTotalPerson() <<","<<  currentFloor << "->" ;// << monitor->printDestList() << endl;
            monitor->printDestList();
        }



        //bu kattakileri indir
        for(int i=0;i < destinationById.size() ; i++){

           if(destinationById[i]==currentFloor && monitor->isInElevator(i)){
               monitor->removeFromElevator(i); //asansörden indir
               monitor->personServed(i);  //tag as served
               indirilen++;
               //usleep(InOutTime);
                if(priorities[i]==1)
                   cout << "Person (" << i <<",hp,"<< initialById[i] <<","<< destinationById[i] <<","<< weights[i] << ") has left the elevator" << endl;
                if(priorities[i]==2)
                   cout << "Person (" << i <<",lp,"<< initialById[i] <<","<< destinationById[i] <<","<< weights[i] << ") has left the elevator" << endl;

           }

            }
            if(dest == currentFloor){
                monitor->newDest();
        }

        //bu kattakileri bindir
        for(int i=0; i < initialById.size() ; i++){
           if(initialById[i]==currentFloor && !monitor->isInElevator(i) && monitor->canGoIn(i) && unservedpeople[i] && orders[i] ){
                monitor->insertToElevator(i);
                //usleep(InOutTime);
                if(priorities[i]==1)
                cout << "Person (" << i << ","<< "hp"  << ","<< initialById[i] <<","<< destinationById[i] <<","<< weights[i] << ") entered the elevator" << endl;
               if(priorities[i]==2)
                   cout << "Person (" << i << ","<< "lp" << ","<< initialById[i] <<","<< destinationById[i] <<","<< weights[i] << ") entered the elevator" << endl;
                monitor->setDestination(destinationById[i]);

                key=1;
            }

           //tekrar request vermesi lazım
           if(initialById[i]==currentFloor && !monitor->isInElevator(i) && !monitor->canGoIn(i)){
                orders[i]=0;
           }

        }

        dest = monitor->getDest();
        if(dest == -1 ){
            state =0;
        }

        monitor->updateDestList();

        if(key){
            if(state==0){
                cout << "Elevator (Idle," << monitor->getTotalWeight() <<","<< monitor->getTotalPerson() <<","<<  currentFloor << "->" ;
                monitor->printDestList();
            }
            if(state==1){
                cout << "Elevator (Moving-up," << monitor->getTotalWeight() <<","<< monitor->getTotalPerson() <<","<<  currentFloor << "->" ;
                monitor->printDestList();
            }
            if(state==2){
                cout << "Elevator (Moving-down," << monitor->getTotalWeight() <<","<< monitor->getTotalPerson() <<","<<  currentFloor << "->" ;
                monitor->printDestList();
            }
            key=0;
        }


        monitor->finish_el();

    }
    cout << "Elevator (Idle," << monitor->getTotalWeight() <<","<< monitor->getTotalPerson() <<","<<  currentFloor << "->" ;
    monitor->printDestList();
    usleep(IDLETime);

   return  0;
}






int main(int argc, char** argv){

//**************************************taking first line of input *****************************************************
    string txtPath= argv[1];
    string line;
    ifstream myfile (txtPath);
    getline (myfile,line);
    istringstream l(line);
    l >> numberOfFloors >> numberOfPeople >> weightCapacity >> personCapacity >> travelTime >> IDLETime >> InOutTime;

//**********************************creating  person threads ****************************
    pthread_t *persons;
    PParam *bparams= new PParam[numberOfPeople+1] ;
    Elevator bs(numberOfPeople);
    persons = new pthread_t[numberOfPeople+1];
    int personWeight=0;
    int initialFloor=0;
    int destinationFloor=0;
    int priority=0;

    int i =0;
    while ( getline (myfile,line) )
    {
        bparams[i].bid = i;
        bparams[i].bs = &bs;

        istringstream l(line);
        l >> personWeight >> initialFloor >> destinationFloor >> priority;

        bparams[i].Priority=priority;

        initialById.push_back(initialFloor);
        destinationById.push_back(destinationFloor);
        weights.push_back(personWeight);
        orders.push_back(0);

        priorities.push_back(priority);
        unservedpeople.push_back(1);


        pthread_create(&persons[i], nullptr, rider, (void *) (bparams + i));

        i++;
    }

    myfile.close();
//*********************************************Creating elevator controller *************************************

    bparams[numberOfPeople].bid = numberOfPeople;
    bparams[numberOfPeople].bs = &bs;
    pthread_create(&persons[numberOfPeople], nullptr, elevator_controller, (void *) (bparams + numberOfPeople));

//********************************************* join *************************************
    for (int i = 0; i < numberOfPeople+1; i++) {
        pthread_join(persons[i], nullptr);
    }


    return 0;
}





