#include "TcpCommunicator.h"

namespace tcp {

Communicator::Communicator(Connection& conn)
    : conn_(&conn)
{
    // Default handlers, does nothing
    msgCb_ = [](const std::string&) {
    };

    conn_->onRead([this](const char* data, size_t size) {
        onRead(data, size);
    });

    conn_->onSent([this](size_t size) {
        onSent(size);
    });
}


Communicator::~Communicator() {}


bool Communicator::onMsg(MsgCb msgCb)
{
    if (started_ || !msgCb)
    {
        return false;
    }

    msgCb_ = std::move(msgCb);
    return true;
}


void Communicator::start()
{
    if (started_ || !conn_)
    {
        return;
    }

    started_ = true;
    conn_->read();
}


void Communicator::send(std::string_view msg)
{
    data::StringSource src(msg);
    data::Packer packer(src);

    wq_.emplace();
    while (packer.get(wq_.back()) > 0)
        ;

    sendInternal();
}


const std::string& Communicator::sendBuf() const
{
    return wq_.front();
}


void Communicator::onRead(const char* data, size_t size)
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


void Communicator::onSent(size_t size)
{
    off_ += size;
    size_t remBytes = sendBuf().size() - off_;

    if (remBytes > 0)
    {
        conn_->write(sendBuf().data() + off_, remBytes);
    }
    else
    {
        wq_.pop();
        sending_ = false;
        sendInternal();
    }
}


void Communicator::sendInternal()
{
    if (sending_ || wq_.empty() || !conn_)
    {
        return;
    }

    sending_ = true;
    off_ = 0;
    conn_->write(sendBuf());
}

} // namespace tcp