
#include "log.h"
#include "buff.h"
#include "tcp.h"
#include "server.h"

string httpRsp(int errCode) {
    ostringstream   os;
    string body = (errCode == 0 ? "ok" : "error");
    os << "HTTP/1.1 " << (errCode == 0 ? 200 : 500) << " \r\n" 
       << "Content-Length: " << body.size() << "\r\n\r\n" << body;

    return os.str();
};

int doHttpGet(Buff * buff, Tcp * tcp) {
    if (buff == nullptr || tcp == nullptr) {
        return -1;
    }
    uint32_t size   = 0;
    char * req = buff->GetPopBuffUntilStr("\r\n\r\n", size);
    if (req != nullptr) {
        tcp->SendMsg((const string)httpRsp(0));
    } else {
        tcp->SendMsg((const string)httpRsp(-1));
    }
    tcp->SendFin();
    return 0;
};

int main() {
    Server   svr;
    if (svr.Open(1234, 10) != 0) {
        ErrorLog(0).Flush();
    }
    svr.SetIoReadFunc([](Buff * buff, Tcp * tcp){
        return doHttpGet(buff, tcp);
    });
    svr.SetExitFunc([]{
        cout << "server exit!" << endl;
    });
    svr.Run();
    return 0;
}


