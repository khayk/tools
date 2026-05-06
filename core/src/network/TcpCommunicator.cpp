#include <core/network/TcpCommunicator.h>
#include <core/network/data/Unpacker.h>
#include <core/network/data/Packer.h>
#include <core/network/data/StringSource.h>

#include <queue>

namespace tcp {

class Communicator::Impl
{
public:
    explicit Impl(Connection& conn)
        : conn_(&conn)
    {
        // Default handler, does nothing
        msgCb_ = [](const std::string&) {};

        conn_->onRead([this](const char* data, size_t size) {
            onRead(data, size);
        });

        conn_->onSent([this](size_t size) {
            onSent(size);
        });
    }

    bool onMsg(MsgCb msgCb)
    {
        if (started_ || !msgCb)
        {
            return false;
        }

        msgCb_ = std::move(msgCb);
        return true;
    }


    void start()
    {
        if (started_ || !conn_)
        {
            return;
        }

        started_ = true;
        conn_->read();
    }


    void sendAsync(std::string_view msg, SentCb sentCb)
    {
        data::StringSource src(msg);
        data::Packer packer(src);

        wq_.emplace("", std::move(sentCb));
        while (packer.get(wq_.back().first) > 0)
        {
            // Keep packing until the buffer is full
        }

        sendInternal();
    }

private:
    MsgCb msgCb_;
    Connection* conn_ {nullptr};
    data::Unpacker unpacker_;
    std::queue<std::pair<std::string, SentCb>> wq_;
    std::string msg_;
    size_t off_ {0};
    bool started_ {false};
    bool sending_ {false};

    [[nodiscard]] const std::string& sendBuf() const
    {
        return wq_.front().first;
    }


    void onRead(const char* data, size_t size)
    {
        unpacker_.put(std::string_view(data, size));

        while (unpacker_.get(msg_) != data::Unpacker::Status::NeedMore)
        {
            if (unpacker_.status() == data::Unpacker::Status::Ready)
            {
                msgCb_(msg_);
                msg_.clear();
            }
        }

        conn_->read();
    }


    void onSent(size_t size)
    {
        off_ += size;
        size_t remBytes = sendBuf().size() - off_;

        if (remBytes > 0)
        {
            conn_->write(sendBuf().data() + off_, remBytes);
        }
        else
        {
            wq_.front().second(true);
            wq_.pop();
            sending_ = false;
            sendInternal();
        }
    }


    void sendInternal()
    {
        if (sending_ || wq_.empty() || !conn_)
        {
            return;
        }

        sending_ = true;
        off_ = 0;
        conn_->write(sendBuf());
    }
};

Communicator::Communicator(Connection& conn)
    : pimpl_(std::make_unique<Impl>(conn))
{
}

Communicator::~Communicator() = default;

bool Communicator::onMsg(MsgCb msgCb)
{
    return pimpl_->onMsg(std::move(msgCb));
}

void Communicator::start()
{
    pimpl_->start();
}

void Communicator::sendAsync(std::string_view data, SentCb sentCb)
{
    pimpl_->sendAsync(data, std::move(sentCb));
}


} // namespace tcp