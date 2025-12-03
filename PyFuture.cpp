class PyFuture {
public:
    PyFuture(std::future<std::string>&& fut)
        : fut_(std::move(fut)) {}

    bool done() {
        return fut_.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    std::string result() {
        return fut_.get();
    }

private:
    std::future<std::string> fut_;
};