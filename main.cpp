#ifndef __PROGTEST__
#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <set>
#include <list>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <stack>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "progtest_solver.h"
#include "sample_tester.h"
using namespace std;
#endif /* __PROGTEST__ */

class CCargoPlanner
{
private:
    int v_saleThreads;
    int v_workerThreads;

    vector<thread> workerThreads;
    vector<thread>   saleThreads;

    vector<ACustomer> customers;
    condition_variable newShip;
    condition_variable newLoad;
    mutex shipMutex , cargoMutex;

    bool stopWorking;
    queue<pair<AShip, bool>>  ships;
    queue<pair<vector<CCargo>, AShip>> toLoad;

public:
    CCargoPlanner(){
        v_saleThreads = 0;
        v_workerThreads = 0;
        stopWorking = false;
    }
    static int SeqSolver ( const vector<CCargo> & cargo, int maxWeight, int maxVolume, vector<CCargo>  & load ){
        return ProgtestSolver(cargo, maxWeight, maxVolume , load);
    }
    void Start ( int sales, int workers ){
        v_saleThreads = sales;
        v_workerThreads = workers;
        for (int i = 0 ; i < sales ; i++)
            saleThreads.emplace_back( &CCargoPlanner::SaleWorker , this );
        for (int i = 0 ; i < workers ; i++)
            workerThreads.emplace_back( &CCargoPlanner::LoadWorker , this );
    }
    void Customer ( ACustomer customer ){
        customers.emplace_back(customer);
    }
    void Stop ( void ){
        shipMutex.lock();
            for (int i = 0 ; i < v_saleThreads ; i++) {
                ships.emplace(AShip(), true);
                newShip.notify_one();
            }
        shipMutex.unlock();

        for (auto & x : workerThreads)
            x.join();
        for (auto & x : saleThreads)
            x.join();
    }
    void Ship ( AShip ship ){
        shipMutex.lock();
        ships.emplace(make_pair(ship , false));
        shipMutex.unlock();
        newShip.notify_one();
    }
    void SaleWorker(){
        AShip tmpShip;
        while(true){
            unique_lock<mutex> lock1 (shipMutex);
            newShip.wait(lock1 , [ this ]{ return !ships.empty(); });
            if (ships.front().second){
                ships.pop();
                if ( ships.empty() ) {
                    cargoMutex.lock();
                    stopWorking = true;
                    cargoMutex.unlock();
                    newLoad.notify_all();
                }
                break;
            }

            tmpShip = ships.front().first;
            ships.pop();
            lock1.unlock();
            vector <CCargo> tmpLoad;
            vector <CCargo> tmpLoadFinal;
            for (auto & x : customers){
                x->Quote(tmpShip->Destination() , tmpLoad );
                tmpLoadFinal.insert(tmpLoadFinal.end() , tmpLoad.begin() , tmpLoad.end());
            }
            cargoMutex.lock();
                toLoad.emplace(tmpLoadFinal , tmpShip);
            cargoMutex.unlock();
            newLoad.notify_one();
        }
    }
    void LoadWorker(){
        while(true){
            unique_lock<mutex> lock (cargoMutex);
            newLoad.wait(lock , [ this ]{ return !toLoad.empty() || stopWorking; });
            if (stopWorking && toLoad.empty())
                break;
            if (toLoad.empty() && !stopWorking)
                continue;
            vector<CCargo> toSolve = toLoad.front().first;
            AShip toShip = toLoad.front().second;
            toLoad.pop();
            lock.unlock();

            vector<CCargo> SolvedLoad;
            SeqSolver(toSolve,toShip->MaxWeight(), toShip->MaxVolume(), SolvedLoad);
            toShip->Load(SolvedLoad);
        }
    }
};

#ifndef __PROGTEST__
int main ( void )
{
    while(true) {
        CCargoPlanner test;
        vector<AShipTest> ships;
        vector<ACustomerTest> customers{make_shared<CCustomerTest>(), make_shared<CCustomerTest>()};

        ships.push_back(g_TestExtra[0].PrepareTest("New York", customers));
        ships.push_back(g_TestExtra[1].PrepareTest("Barcelona", customers));
        ships.push_back(g_TestExtra[2].PrepareTest("Kobe", customers));
        ships.push_back(g_TestExtra[8].PrepareTest("Perth", customers));
        // add more ships here

        for (auto x : customers)
            test.Customer(x);

        test.Start(4, 2);

        for (auto x : ships)
            test.Ship(x);

        test.Stop();

        for (auto x : ships)
            cout << x->Destination() << ": " << (x->Validate() ? "ok" : "fail") << endl;
    }
  return 0;
}
#endif /* __PROGTEST__ */
