#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSurfaceFormat>
#include "BGFXItem.h"

int main(int argc, char *argv[])
{
    // 设置Qt属性
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    
    QGuiApplication app(argc, argv);
    
    // 设置OpenGL格式
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setMajorVersion(3);
    format.setMinorVersion(3);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSwapInterval(1); // VSync
    QSurfaceFormat::setDefaultFormat(format);
    
    // 设置场景图后端
    QQuickWindow::setSceneGraphBackend(QSGRendererInterface::OpenGL);
    
    // 注册QML类型
    qmlRegisterType<BGFXItem>("BGFXModule", 1, 0, "BGFXItem");
    
    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    
    if (engine.rootObjects().isEmpty())
        return -1;
    
    return app.exec();
}