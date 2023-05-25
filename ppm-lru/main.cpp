#include "PPMR.hpp"

int main(int argc, const char * argv[])
{
    std::string path    = std::string(argv[1]);
    std::string dataset = std::string(argv[2]);
    int         order   = std::atoi(argv[3]);
    long        lim     = std::atol(argv[4]);
    
    PPMR ppmr;
    ppmr.set_order_limit(order);
    ppmr.set_node_limit(lim);
    
    ppmr.run(path, dataset);
    std::cout << ppmr.stats() << std::endl;
    
    return 0;
}

