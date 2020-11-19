#include "scene.h"

Scene::Scene(QObject *parent)
    : QGraphicsScene(parent)
{
    setSceneRect(QRectF(0, 0, 300, 300));
}