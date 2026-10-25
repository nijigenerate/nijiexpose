#include "control_output.hpp"
int main(int argc,char** argv) {
    if(argc!=2)return 2;
    return nijiexpose::control::writeResponse(argv[1],"audit.response.json",
        "{\"ok\":true}")?0:1;
}
