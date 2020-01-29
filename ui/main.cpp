#include "mainwindow.h"
#include <QApplication>

#include "rapi/rapi.h"

#include "interface/serial_port/serial_interface_provider.h"
#include "tools/queue_threadsafe.h"

#include "asuit_protocol.h"

static auto &rapi_instance = RAPI::instance();

std::queue_threadsafe<char> q_log;

void RAPI::set_parser_cb(AbstractInterface* iface)
{
    auto *r = new ASuitProtocol(iface->get_id(), iface->get_if_impl_mutex(), q_log);
    iface->set_parser(r);
    iface->get_ptr_impl()->set_tx(std::bind(&AbstractInterface::tx_data, iface, std::placeholders::_1));
}

void RAPI::setup_interfaces()
{
    interface_providers.emplace_back(std::unique_ptr<AbstractInterfaceProvider>(new SerialInterfaceProvider(interface_collector)));
//    interface_providers.emplace_back(std::unique_ptr<AbstractInterfaceProvider>(new UdpInterfaceProvider(interface_collector)));
}



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w(q_log);
    w.show();

    return a.exec();
}
