#include "train_client.h"
#include <QJsonDocument>
#include <QJsonObject>

// ========== TrainClientWorker ==========

TrainClientWorker::TrainClientWorker(int port, QObject *parent)
    : QObject(parent), m_port(port),
      m_ctx(1),
      m_req(m_ctx, zmq::socket_type::req),
      m_sub(m_ctx, zmq::socket_type::sub)
{
}

TrainClientWorker::~TrainClientWorker()
{
    m_pollTimer.stop();
    m_req.close();
    m_sub.close();
    m_ctx.close();
}

void TrainClientWorker::start()
{
    m_pollTimer.moveToThread(QThread::currentThread());

    int linger = 0;
    m_req.setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
    m_req.connect("tcp://localhost:" + std::to_string(m_port + 1));

    m_sub.setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
    m_sub.connect("tcp://localhost:" + std::to_string(m_port));
    const char *sub_filter = "";
    m_sub.setsockopt(ZMQ_SUBSCRIBE, sub_filter, strlen(sub_filter));

    m_pollTimer.setInterval(2000);
    connect(&m_pollTimer, &QTimer::timeout, this, &TrainClientWorker::pollSubscription);
    m_pollTimer.start();
}

void TrainClientWorker::sl_sendCommand(const QString &cmd)
{
    Json::Value req;
    req["cmd"] = cmd.toStdString();

    Json::FastWriter writer;
    std::string reqStr = writer.write(req);

    try {
        m_req.send(zmq::buffer(reqStr), zmq::send_flags::none);

        zmq::message_t reply;
        auto result = m_req.recv(reply, zmq::recv_flags::none);
        if (!result) {
            Json::Value err;
            err["status"] = "error";
            err["message"] = "No response from server";
            emit sg_commandResult(err);
            return;
        }

        std::string replyStr = static_cast<char*>(reply.data());
        Json::Value resp;
        Json::Reader reader;
        reader.parse(replyStr, resp);
        emit sg_commandResult(resp);
    } catch (const zmq::error_t &e) {
        Json::Value err;
        err["status"] = "error";
        err["message"] = e.what();
        emit sg_commandResult(err);
    }
}

void TrainClientWorker::pollSubscription()
{
    zmq::pollitem_t items[] = { {m_sub, 0, ZMQ_POLLIN, 0} };
    try {
        zmq::poll(items, 1, std::chrono::milliseconds(0));
        if (items[0].revents & ZMQ_POLLIN) {
            zmq::message_t topic;
            zmq::message_t data;
            m_sub.recv(topic, zmq::recv_flags::none);
            m_sub.recv(data, zmq::recv_flags::none);

            std::string topicStr = static_cast<char*>(topic.data());
            std::string dataStr = static_cast<char*>(data.data());

            Json::Value msg;
            Json::Reader reader;
            reader.parse(dataStr, msg);

            if (topicStr == "metrics") {
                TrainMetrics m;
                m.epoch = msg["epoch"].asInt();
                m.totalEpochs = msg["total_epochs"].asInt();
                m.loss = msg["loss"].asDouble();
                m.boxLoss = msg["box_loss"].asDouble();
                m.clsLoss = msg["cls_loss"].asDouble();
                m.dflLoss = msg["dfl_loss"].asDouble();
                m.precision = msg["precision"].asDouble();
                m.recall = msg["recall"].asDouble();
                m.mAP50 = msg["mAP50"].asDouble();
                m.mAP50_95 = msg["mAP50_95"].asDouble();
                emit sg_metrics(m);
            } else if (topicStr == "train_end") {
                emit sg_trainEnd(QString::fromStdString(msg["save_dir"].asString()));
            } else if (topicStr == "error") {
                emit sg_error(QString::fromStdString(msg["message"].asString()));
            }
        }
    } catch (const zmq::error_t &) {
    }
}

// ========== TrainClient ==========

TrainClient::TrainClient(int port, QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<Json::Value>("Json::Value");

    m_worker = new TrainClientWorker(port);
    m_worker->moveToThread(&m_thread);

    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &TrainClientWorker::sg_commandResult, this, &TrainClient::sg_commandResult);
    connect(m_worker, &TrainClientWorker::sg_metrics, this, &TrainClient::sg_metrics);
    connect(m_worker, &TrainClientWorker::sg_trainEnd, this, &TrainClient::sg_trainEnd);
    connect(m_worker, &TrainClientWorker::sg_error, this, &TrainClient::sg_error);

    connect(this, &TrainClient::sg_sendCommand, m_worker, &TrainClientWorker::sl_sendCommand);
    connect(this, &TrainClient::sg_startWorker, m_worker, &TrainClientWorker::start);

    m_thread.start();
    emit sg_startWorker();
}

TrainClient::~TrainClient()
{
    m_thread.quit();
    m_thread.wait(3000);
}

void TrainClient::sendCommand(const QString &cmd)
{
    emit sg_sendCommand(cmd);
}
