#include <chrono>

/* Only needed for the sake of this example. */
#include <iostream>
#include <thread>

    
using namespace std;
using namespace std::chrono;

int main()
{
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::duration;
    using std::chrono::milliseconds;

  
   


    static int secret = 6;
    static int _x;
    static int fuite;

    auto start = high_resolution_clock::now();
    //cout << " start "<< start<<"\n";
    try {
        int cpt = 0;
        //clock_t startloop = clock();
        while (cpt < 10) {


            if ((secret - cpt) == 0){
                throw std::overflow_error("Divide by zero exception from throw");
             }
            else {
                _x = 1 / (secret - cpt);
               
                cpt++;
                cout << "round " << cpt << " threadId " << this_thread::get_id()<<"\n";
                
                this_thread::sleep_for(chrono::milliseconds(100));
                
            }
            

        }
    }
    catch (const std::exception& e) {
      
        cerr << e.what()<<"\n";
    }  
    auto end = high_resolution_clock::now();
    auto ms_int = duration_cast<milliseconds>(end - start);

    fuite = (int)(ms_int.count() / 100);
    cout << "duration : " << ms_int.count() << " ms | fuite = " << fuite << " leak ?\n" << endl;


   
    return 0;
}