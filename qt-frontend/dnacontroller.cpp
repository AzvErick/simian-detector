#include "dnacontroller.h"

#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>

DnaController::DnaController(QObject* parent)
    : QObject(parent)
    , networkManager_(new QNetworkAccessManager(this))
    , apiUrl_("http://localhost:8080")
    , loading_(false)
{
}

void DnaController::setApiUrl(const QString& url) {
    if (apiUrl_ != url) {
        apiUrl_ = url;
        emit apiUrlChanged();
    }
}

void DnaController::setLoading(bool loading) {
    if (loading_ != loading) {
        loading_ = loading;
        emit loadingChanged();
    }
}

// Monta o JSON e faz POST /simian
// Resposta chega assincrona no onAnalyzeReply
void DnaController::analyzeDna(const QStringList& dna) {
    setLoading(true);

    QJsonArray dnaArray;
    for (const auto& row : dna) {
        dnaArray.append(row.toUpper());
    }

    QJsonObject json;
    json["dna"] = dnaArray;

    QNetworkRequest request(QUrl(apiUrl_ + "/simian"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = networkManager_->post(request, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onAnalyzeReply(reply);
    });
}

// Processa resposta do POST /simian
// Converte JSON pra QVariantMap
void DnaController::onAnalyzeReply(QNetworkReply* reply) {
    setLoading(false);

    QVariantMap result;

    if (reply->error() == QNetworkReply::ConnectionRefusedError) {
        result["error"] = "Cannot connect to API. Is the server running on " + apiUrl_ + "?";
        result["isSimian"] = false;
        result["matches"] = QVariantList();
        emit analysisComplete(result);
        reply->deleteLater();
        return;
    }

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    if (statusCode == 400) {
        result["error"] = obj["error"].toString();
        result["isSimian"] = false;
        result["matches"] = QVariantList();
    } else {
        result["error"] = "";
        result["isSimian"] = obj["is_simian"].toBool();

        // Converte matches pra formato que o QML entende
        QVariantList matches;
        QJsonArray matchArray = obj["matches"].toArray();
        for (const auto& m : matchArray) {
            QJsonObject match = m.toObject();
            QVariantMap matchMap;
            matchMap["startRow"] = match["start"].toArray()[0].toInt();
            matchMap["startCol"] = match["start"].toArray()[1].toInt();
            matchMap["endRow"] = match["end"].toArray()[0].toInt();
            matchMap["endCol"] = match["end"].toArray()[1].toInt();
            matchMap["letter"] = match["letter"].toString();
            matchMap["direction"] = match["direction"].toString();
            matches.append(matchMap);
        }
        result["matches"] = matches;
    }

    emit analysisComplete(result);
    reply->deleteLater();
}

void DnaController::fetchStats() {
    setLoading(true);

    QNetworkRequest request(QUrl(apiUrl_ + "/stats"));
    QNetworkReply* reply = networkManager_->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onStatsReply(reply);
    });
}

void DnaController::onStatsReply(QNetworkReply* reply) {
    setLoading(false);

    QVariantMap stats;

    if (reply->error() != QNetworkReply::NoError) {
        stats["error"] = "Cannot connect to API";
        emit statsReceived(stats);
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    stats["error"] = "";
    stats["countSimian"] = obj["count_mutant_dna"].toInt();
    stats["countHuman"] = obj["count_human_dna"].toInt();
    stats["ratio"] = obj["ratio"].toDouble();

    emit statsReceived(stats);
    reply->deleteLater();
}

void DnaController::checkHealth() {
    QNetworkRequest request(QUrl(apiUrl_ + "/health"));
    QNetworkReply* reply = networkManager_->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onHealthReply(reply);
    });
}

void DnaController::onHealthReply(QNetworkReply* reply) {
    QVariantMap health;

    if (reply->error() != QNetworkReply::NoError) {
        health["status"] = "offline";
        health["database"] = false;
    } else {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject obj = doc.object();
        health["status"] = obj["status"].toString();
        health["database"] = obj["database"].toBool();
    }

    emit healthChecked(health);
    reply->deleteLater();
}
