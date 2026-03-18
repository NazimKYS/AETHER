






















void foo(){
    int x,y,z;
    x=1;
    y=0;
    while (y<5){
        x=x*2; // <-- TS assert(x<=30)
        y++;   
    }
    z=x;
}


/*#include <cstdlib>
#include <iostream>
//#include"Log.h"
#include <string>
//#include <Windows.h>
#include <thread>
#include <time.h>
#include <chrono>

#include <stdexcept>

// Integer division/remainder, catching divide by zero.


using namespace std;
using namespace std::chrono;


int main()
{

    static int secret = 6;
    static int _x;
    static int fuite;
    std::clock_t start = std::clock();
    cout << " start "<< start<<"\n";
    try {
        int cpt = 0;
        //clock_t startloop = clock();
        while (cpt < 10) {


            if ((secret - cpt) == 0){
                throw std::overflow_error("Divide by zero exception from throw");
             }
            else {
                _x = 1 / (secret - cpt);
                //throw(_x);
                cpt++;
                cout << "round " << cpt << " threadId " << this_thread::get_id()<<"\n";
                clock_t startSleep = clock();
                this_thread::sleep_for(chrono::milliseconds(100));
                clock_t endSleep = clock();
                cout<<"sleep duration : "<<(endSleep-startSleep)<<"\n";
            }
            //m functionDecl(hasDescendant(callExpr().bind("clock")
            
        }
    }
    catch (const std::exception& e) {
        //cout << "exception\n";
        cerr << e.what()<<"\n";
    }
    std::clock_t end = std::clock();
    cout << " end "<< end<<"\n";
    fuite = (int)((end - start) / 100);
    cout << "duration : " << (end - start) << " ms | fuite = " << fuite << " leak ?\n" << endl;


    //auto end = chrono::steady_clock::now();
   
    //auto diff = end - start;
    //double elapsed = double(end - start);

    //instanceOfErrorSleep.fuite = (int)(diff / 100);
    //cout << chrono::duration <double, milli>(diff).count() << " ms\n" << endl;
    

    std::cin.get();


}
*/
