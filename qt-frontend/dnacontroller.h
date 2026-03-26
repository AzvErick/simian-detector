#ifndef DNACONTROLLER_H
#define DNACONTROLLER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>

// Ponte entre o QML (interface) e a API REST (backend)
// QML chama os metodos Q_INVOKABLE, e recebe resultados via signals
//
// Fluxo: QML chama analyzeDna() -> C++ faz POST -> emite analysisComplete() -> QML atualiza tela
class DnaController : public QObject {
    Q_OBJECT

    // Propriedades acessiveis no QML, controller.loading, controller.apiUrl
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString apiUrl READ apiUrl WRITE setApiUrl NOTIFY apiUrlChanged)

public:
    explicit DnaController(QObject* parent = nullptr);

    bool loading() const { return loading_; }
    QString apiUrl() const { return apiUrl_; }
    void setApiUrl(const QString& url);

    // Q_INVOKABLE = QML consegue chamar essas funcoes
    Q_INVOKABLE void analyzeDna(const QStringList& dna);
    Q_INVOKABLE void fetchStats();
    Q_INVOKABLE void checkHealth();

signals:
    // Signals que o QML escuta via Connections { target: controller }
    void analysisComplete(QVariantMap result);
    void statsReceived(QVariantMap stats);
    void healthChecked(QVariantMap health);
    void networkError(QString message);
    void loadingChanged();
    void apiUrlChanged();

private slots:
    void onAnalyzeReply(QNetworkReply* reply);
    void onStatsReply(QNetworkReply* reply);
    void onHealthReply(QNetworkReply* reply);

private:
    QNetworkAccessManager* networkManager_;
    QString apiUrl_;
    bool loading_;

    void setLoading(bool loading);
};

#endif
