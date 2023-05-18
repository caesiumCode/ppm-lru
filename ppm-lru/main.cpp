#include "PPMC.hpp"

using Timer         = std::chrono::high_resolution_clock;
using TimerMeasure  = std::chrono::time_point<Timer>;

int main(int argc, const char * argv[])
{
    std::string path    = std::string(argv[1]);
    std::string dataset = std::string(argv[2]);
    
    PPMC ppmc;
    ppmc.set_order(3);
    
    TimerMeasure START = Timer::now();
    ppmc.run(path, dataset);
    TimerMeasure END = Timer::now();
    
    std::cout << "file name: " << dataset << std::endl << std::endl;
    std::cout << std::setw(20) << "total time (min)" << std::setw(12) << std::chrono::duration<double>(END - START).count() / 60 << std::endl;
    ppmc.disp_stats();
    std::cout << std::endl;
    
    return 0;
}

