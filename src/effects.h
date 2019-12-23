#include <sp2/tween.h>

class ScaredEffect : public sp::Node
{
public:
    ScaredEffect(sp::P<sp::Node> parent)
    : sp::Node(parent)
    {
        buildString(render_data, "\\|/");
        setPosition(sp::Vector2d(0, 1.2));
    }

    virtual void onFixedUpdate() override
    {
        if (lifetime)
            lifetime--;
        else
            delete this;
    }

private:
    int lifetime = 30;
};

class FireEffect : public sp::Node
{
public:
    FireEffect(sp::P<sp::Node> parent)
    : sp::Node(parent)
    {
        buildString(render_data, "*");
        max_lifetime = sp::irandom(50, 150);
        lifetime = max_lifetime;
        velocity = sp::Vector2d(sp::random(0.1, 1.0), 0).rotate(sp::random(0, 360));
    }

    virtual void onFixedUpdate() override
    {
        render_data.color = sp::Tween<sp::Color>::easeOutCubic(lifetime, max_lifetime, 0, sp::HsvColor(0, 100, 100), sp::HsvColor(30, 100, 100));
        render_data.color.a = sp::Tween<float>::easeOutCubic(lifetime, max_lifetime, max_lifetime / 2, 1.0f, 0.0f);
        setPosition(getPosition2D() + velocity * 0.1);
        velocity *= 0.99;
        if (lifetime)
            lifetime--;
        else
            delete this;
    }

private:
    int lifetime;
    int max_lifetime;
    sp::Vector2d velocity;
};