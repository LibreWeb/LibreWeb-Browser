#ifndef RENDER_INTERFACE_H
#define RENDER_INTERFACE_H

class Scene;
struct cmark_node;

/**
 * \class RendererI Renderer Abstract Base Class
 */
class RendererI
{
public:
    virtual ~RendererI() {};
    // TODO: Combien those two sets to 1, create an abstract class for setting a scene in Qt and/or Imgui.
    virtual void setScene(Scene *scene) = 0;// For Qt
    virtual void setUnknownYet() = 0; // For Imgui

    virtual void renderDocument(cmark_node *root, int width = 0) = 0;
};
#endif
