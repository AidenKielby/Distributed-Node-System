#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "MasterPythonApi.hpp"
#include "PyFuture.cpp"

namespace py = pybind11;

PYBIND11_MODULE(pctree, m){

    py::class_<PyFuture>(m, "PyFuture")
        .def("done", &PyFuture::done)
        .def("result", &PyFuture::result);

    m.doc() = "PCTree Distributed Node System Python bindings";

    m.def("start_master", &initMaster, "Start the master server");
    m.def("client_list", &listActiveClients, "Return a list of connected clients");
    m.def("send_file_async",
      [](int client_index, const std::string& file_path, const std::string& func_name) {
          return PyFuture(sendToClientAsync(client_index, file_path, func_name));
      },
      py::arg("client_index"), py::arg("file_path"), py::arg("function_name"));
    m.def("ping_client", &pingClient, "Ping client at index", 
        py::arg("client_index"));
    m.def("kill_master", &killMaster, "Kills the master server");
}