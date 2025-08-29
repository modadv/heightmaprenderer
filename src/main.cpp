// main.cpp
#include "quick/bgfx_item.h"
#include "adapters/bgfx_adapter.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QQmlContext>
#include <QOpenGLContext> // <-- 修复3：添加缺失的头文件

int main(int argc, char* argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QQuickWindow::setSceneGraphBackend(QSGRendererInterface::OpenGL);

    QGuiApplication app(argc, argv);

    qmlRegisterType<BgfxItem>("com.example.bgfx", 1, 0, "BgfxItem");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    // 1. 从已加载的QML场景中获取顶层窗口
    QQuickWindow* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    if (!window) {
        qCritical("Failed to get QQuickWindow.");
        return -1;
    }

    // 2. 查找我们用objectName标记的BgfxItem实例
    BgfxItem* bgfxItem = window->findChild<BgfxItem*>("bgfxItem");
    if (!bgfxItem) {
        qCritical("Failed to find BgfxItem with objectName 'bgfxItem'.");
        return -1;
    }

    // 3. 等待OpenGL上下文就绪
    QObject::connect(window, &QQuickWindow::openglContextCreated, window, [&](QOpenGLContext* context) {
        static bool initialized = false;
        if (!initialized) {
            initialized = true;

            // 4. 在上下文就绪后，创建Adapter
            BgfxAdapter* adapter = new BgfxAdapter((void*)window->winId(), context->nativeHandle(), &app);

            // 5. 将Adapter实例显式地设置给BgfxItem
            bgfxItem->setAdapter(adapter);
        }
        });

    return app.exec();
}