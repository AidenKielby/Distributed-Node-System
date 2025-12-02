#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "MasterPythonApi.hpp"

namespace py = pybind11;

PYBIND11_MODULE(pctree, m){
    m.doc() = "PCTree Distributed Node System Python bindings";

    m.def("start_master", &initMaster, "Start the master server");
    m.def("client_list", &listActiveClients, "Return a list of connected clients");
    m.def("send_file", &sendToClient, "Send a file to a client",
          py::arg("client_index"), py::arg("file_path"), py::arg("function_name"));
    m.def("ping_client", &pingClient, "Ping client at index", 
        py::arg("client_index"));
    m.def("kill_master", &killMaster, "Kills the master server");
}