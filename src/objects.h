
class SpikeTrap : public DungeonObject
{
public:
    SpikeTrap(sp::P<DungeonRoom> room)
    : DungeonObject(room)
    {
        value = cost_map["PIT"];
        buildString(render_data, "^^^");
        render_data.scale = sp::Vector3f(0.6, 0.6, 0.6);
        render_data.color = sp::HsvColor(0, 70, 100);
        render_data.order = 1;
        setPosition(sp::Vector2d(0, -0.5));
    }

    virtual void onEnteredRoom(sp::P<Adventurer> adventurer) override
    {
        if (body)
        {
            adventurer->addFear(1);
            new ScaredEffect(adventurer);
        }
    }

    virtual void onCenterRoom(sp::P<Adventurer> adventurer) override
    {
        if (active)
        {
            active = false;
            if (adventurer->takeDamage(1))
            {
                body = new sp::Node(getParent());
                buildString(body->render_data, "@");
                body->render_data.scale = sp::Vector3f(1.5, 1.5, 1.5);
                body->render_data.color = sp::HsvColor(0, 80, 70);
                body->render_data.order = 2;
                body->setRotation(90);

                adventurer_results.push_back({AdventurerResult::Death, adventurer->level, adventurer->loot + 20 + adventurer->level * 30, float(adventurer->level) * 1.5f, 0.0f, 0.0f});
            }
        }
    }

    virtual void onEndOfDay() override
    {
        active = true;
        if (body)
            placable_bodies += 1;
        body.destroy();
    }

private:
    bool active = true;
    sp::P<sp::Node> body;
};


class Loot : public DungeonObject
{
public:
    Loot(sp::P<DungeonRoom> room)
    : DungeonObject(room)
    {
        value = cost_map["LOOT"];
        buildString(render_data, "%");
        render_data.scale = sp::Vector3f(1.1, 1.1, 1.1);
        render_data.color = sp::HsvColor(60, 100, 100);
        render_data.order = 1;
    }

    virtual void onEnteredRoom(sp::P<Adventurer> adventurer) override
    {
    }

    virtual void onCenterRoom(sp::P<Adventurer> adventurer) override
    {
        adventurer->loot += 100;
        adventurer->addFear(1);
        delete this;
    }

    virtual void onEndOfDay() override
    {
    }

private:
};

class Body : public DungeonObject
{
public:
    Body(sp::P<DungeonRoom> room)
    : DungeonObject(room)
    {
        buildString(render_data, "@");
        render_data.scale = sp::Vector3f(1.5, 1.5, 1.5);
        render_data.color = sp::HsvColor(0, 80, 70);
        render_data.order = 2;
        setRotation(90);
    }

    virtual void onEnteredRoom(sp::P<Adventurer> adventurer) override
    {
        adventurer->addFear(1);
        new ScaredEffect(adventurer);
    }

    virtual void onCenterRoom(sp::P<Adventurer> adventurer) override
    {
    }

    virtual void onEndOfDay() override
    {
        decay--;
        render_data.color = sp::HsvColor(0, 80, 20 + decay * 10);
        if (!decay)
            delete this;
    }

private:
    int decay = 5;
};

class Slime : public DungeonObject
{
public:
    Slime(sp::P<DungeonRoom> room)
    : DungeonObject(room)
    {
        buildString(render_data, "&%$");
        render_data.color = sp::HsvColor(120, 80, 90);
        render_data.order = 2;
        setRotation(90);
    }

    virtual void onEnteredRoom(sp::P<Adventurer> adventurer) override
    {
        adventurer->addSlime();
        new ScaredEffect(adventurer);
    }

    virtual void onCenterRoom(sp::P<Adventurer> adventurer) override
    {
    }

    virtual void onEndOfDay() override
    {
        decay--;
        render_data.color = sp::HsvColor(0, 80, 20 + decay * 10);
        if (!decay)
            delete this;
    }

private:
    int decay = 5;
};

class FireTrap : public DungeonObject
{
public:
    FireTrap(sp::P<DungeonRoom> room)
    : DungeonObject(room)
    {
        value = cost_map["FIRE"];
        buildString(render_data, "> <");
        render_data.scale = sp::Vector3f(0.8, 0.8, 0.8);
        render_data.color = sp::HsvColor(0, 70, 100);
        render_data.order = 1;
    }

    virtual void onEnteredRoom(sp::P<Adventurer> adventurer) override
    {
        if (body)
        {
            adventurer->addFear(2);
            new ScaredEffect(adventurer);
        }
    }

    virtual void onCenterRoom(sp::P<Adventurer> adventurer) override
    {
        if (active)
        {
            active = false;
            for(int n=0; n<100; n++)
                new FireEffect(this);
            if (adventurer->takeDamage(4))
            {
                body = new sp::Node(getParent());
                buildString(body->render_data, "@");
                body->render_data.scale = sp::Vector3f(1.5, 1.5, 1.5);
                body->render_data.color = sp::HsvColor(0, 10, 50);
                body->render_data.order = 2;
                body->setRotation(90);

                adventurer_results.push_back({AdventurerResult::Death, adventurer->level, adventurer->loot + 20 + adventurer->level * 30, float(adventurer->level) * 2.5f, 0.0f, 1.0f});
            }
        }
    }

    virtual void onEndOfDay() override
    {
        active = true;
        body.destroy();
    }

private:
    bool active = true;
    sp::P<sp::Node> body;
};
