#include <sp2/engine.h>
#include <sp2/window.h>
#include <sp2/logging.h>
#include <sp2/random.h>
#include <sp2/io/directoryResourceProvider.h>
#include <sp2/graphics/fontManager.h>
#include <sp2/graphics/gui/scene.h>
#include <sp2/graphics/gui/theme.h>
#include <sp2/graphics/gui/loader.h>
#include <sp2/graphics/scene/graphicslayer.h>
#include <sp2/graphics/scene/basicnoderenderpass.h>
#include <sp2/graphics/scene/collisionrenderpass.h>
#include <sp2/graphics/textureManager.h>
#include <sp2/scene/scene.h>
#include <sp2/scene/node.h>
#include <sp2/scene/camera.h>
#include <sp2/io/keybinding.h>

#include <unordered_set>

sp::P<sp::Window> window;
sp::P<sp::gui::Widget> main_ui;

sp::io::Keybinding escape_key{"exit", "Escape"};
sp::Font* main_font;

int money = 30;
float risk = 0.0;
float reward = 0.0;
float dragon_deception = 0.0;
int placable_bodies = 0;

std::unordered_map<sp::string, int> cost_map{
    {"DIG", 10},
    {"PIT", 30},
    {"LOOT", 100},
    {"FIRE", 300},
    {"SLIME", 200},
};
struct AdventurerResult
{
    enum Result
    {
        Death,
        Fled,
        Escaped
    } result;
    int level;
    int money;
    float risk;
    float reward;
    float deception;
};

std::vector<AdventurerResult> adventurer_results;

void buildString(sp::RenderData& render_data, const sp::string& str)
{
    static std::unordered_map<sp::string, std::shared_ptr<sp::MeshData>> cache;

    render_data.type = sp::RenderData::Type::Normal;
    render_data.shader = sp::Shader::get("internal:basic.shader");
    auto it = cache.find(str);
    if (it != cache.end())
    {
        render_data.mesh = it->second;
    }
    else
    {
        auto info = main_font->prepare(str, 32, 1.0, sp::Vector2d(0, 0), sp::Alignment::TopLeft);
        sp::Vector2f offset = info.getUsedAreaSize() * 0.5f;
        for(auto& i : info.data)
        {
            i.position.x -= offset.x;
            i.position.y += offset.y;
            i.position.y *= 0.8;
        }
        render_data.mesh = info.create();
        cache[str] = render_data.mesh;
    }
    render_data.texture = main_font->getTexture(32);
}

class DungeonRoom;
class Adventurer;

class DungeonObject : public sp::Node
{
public:
    using sp::Node::Node;

    virtual void onEnteredRoom(sp::P<Adventurer> adventurer) {}
    virtual void onCenterRoom(sp::P<Adventurer> adventurer) {}
    virtual void onEndOfDay() {}

    int value = 0;
};

sp::P<DungeonRoom> getRoomAt(sp::Vector2d position, bool allow_unbuild=false);
class DungeonRoom : public sp::Node
{
public:
    DungeonRoom(sp::P<sp::Node> parent)
    : sp::Node(parent)
    {
        updateGraphics();
    }

    void updateGraphics()
    {
        bool up = build && getRoomAt(getPosition2D() + sp::Vector2d(0, 6)) != nullptr;
        bool down = build && getRoomAt(getPosition2D() + sp::Vector2d(0, -6)) != nullptr;
        bool left = build && getRoomAt(getPosition2D() + sp::Vector2d(-4, 0)) != nullptr;
        bool right = build && getRoomAt(getPosition2D() + sp::Vector2d(4, 0)) != nullptr;

        if (entrance)
            left = true;

        if (build)
            render_data.color = sp::Color(1.0, 1.0, 1.0);
        else
            render_data.color = sp::Color(0.4, 0.4, 0.4);

        sp::string result;
        if (up)
            result += "  | |  \n +- -+ \n";
        else
            result += "       \n +---+ \n";
        if (left && right)
            result += "-|   |-\n       \n-|   |-\n";
        else if (left)
            result += "-|   | \n     | \n-|   | \n";
        else if (right)
            result += " |   |-\n |     \n |   |-\n";
        else
            result += " |   | \n |   | \n |   | \n";
        if (down)
            result += " +- -+ \n  | |  ";
        else
            result += " +---+ \n       ";
        buildString(render_data, result);
    }

    void doBuild()
    {
        if (build)
            return;
        build = true;
        updateGraphics();
        sp::P<DungeonRoom> r = getRoomAt(getPosition2D() + sp::Vector2d(0, 6), true);
        if (r)
            r->updateGraphics();
        else
            (new DungeonRoom(getParent()))->setPosition(getPosition2D() + sp::Vector2d(0, 6));
        r = getRoomAt(getPosition2D() + sp::Vector2d(0, -6), true);
        if (r)
            r->updateGraphics();
        else
            (new DungeonRoom(getParent()))->setPosition(getPosition2D() + sp::Vector2d(0, -6));
        r = getRoomAt(getPosition2D() + sp::Vector2d(-4, 0), true);
        if (r)
            r->updateGraphics();
        else if (getPosition2D().x > 1.0)
            (new DungeonRoom(getParent()))->setPosition(getPosition2D() + sp::Vector2d(-4, 0));
        r = getRoomAt(getPosition2D() + sp::Vector2d(4, 0), true);
        if (r)
            r->updateGraphics();
        else
            (new DungeonRoom(getParent()))->setPosition(getPosition2D() + sp::Vector2d(4, 0));
    }

    bool build = false;
    bool entrance = false;
    sp::P<DungeonObject> main_object;
};

sp::P<DungeonRoom> getRoomAt(sp::Vector2d position, bool allow_unbuild)
{
    for(sp::P<DungeonRoom> room : sp::Scene::get("DUNGEON")->getRoot()->getChildren())
    {
        if (!room || (!room->build && !allow_unbuild))
            continue;
        if ((room->getPosition2D() - position).length() < 2.0)
        {
            return room;
        }
    }
    return nullptr;
}

class Adventurer : public sp::Node
{
public:
    Adventurer(sp::P<sp::Node> parent, int level)
    : sp::Node(parent), level(level)
    {
        buildString(render_data, "@");
        render_data.scale = sp::Vector3f(1.3, 1.3, 1.3);
        render_data.color = sp::HsvColor(90, 70, 100);
        render_data.order = 5;

        setPosition(sp::Vector2d(-4, 0));
        current_room = getRoomAt(sp::Vector2d(0, 0));
        hp = level;
        courage = level + 1;
    }

    virtual void onFixedUpdate() override
    {
        if (current_room && (current_room->getPosition2D() - getPosition2D()).length() < 0.1)
        {
            if (visited_rooms.find(*current_room) == visited_rooms.end())
            {
                for(sp::P<DungeonObject> obj : current_room->getChildren())
                {
                    if (obj)
                        obj->onCenterRoom(this);
                }
                visited_rooms.insert(*current_room);
            }

            sp::PList<DungeonRoom> options;
            sp::P<DungeonRoom> r;
            r = getRoomAt(getPosition2D() + sp::Vector2d(4, 0));
            if (r && visited_rooms.find(*r) == visited_rooms.end())
                options.add(r);
            r = getRoomAt(getPosition2D() + sp::Vector2d(-4, 0));
            if (r && visited_rooms.find(*r) == visited_rooms.end())
                options.add(r);
            r = getRoomAt(getPosition2D() + sp::Vector2d(0, 6));
            if (r && visited_rooms.find(*r) == visited_rooms.end())
                options.add(r);
            r = getRoomAt(getPosition2D() + sp::Vector2d(0, -6));
            if (r && visited_rooms.find(*r) == visited_rooms.end())
                options.add(r);

            if (!fleeing && options.size() > 0)
            {
                int index = sp::irandom(0, options.size() - 1);
                for(auto room : options)
                {
                    if (index)
                    {
                        index--;
                    }
                    else
                    {
                        backtrack_list.add(current_room);
                        current_room = room;
                        in_room = false;
                        break;
                    }
                }
            }
            else if (backtrack_list.size() > 0)
            {
                //TODO: If we are next to a backtrack room, take that one and ignore the rest of the list.
                for(auto room : backtrack_list)
                    current_room = room;
                in_room = false;
                backtrack_list.remove(current_room);
            }
            else
            {
                current_room = nullptr;
            }
        }
        else if (current_room)
        {
            double speed = 0.08;
            if (fleeing) speed *= 1.5;
            setPosition(getPosition2D() + (current_room->getPosition2D() - getPosition2D()).normalized() * speed);
            if (!in_room && (current_room->getPosition2D() - getPosition2D()).length() < 1.0)
            {
                in_room = true;
                if (visited_rooms.find(*current_room) == visited_rooms.end())
                {
                    for(sp::P<DungeonObject> obj : current_room->getChildren())
                    {
                        if (obj)
                            obj->onEnteredRoom(this);
                    }

                    if (fleeing)
                    {
                        for(auto room : backtrack_list)
                            current_room = room;
                        in_room = false;
                        backtrack_list.remove(current_room);
                    }
                }
            }
        }
        else
        {
            setPosition(getPosition2D() + sp::Vector2d(-1, 0) * 0.1);
            if (getPosition2D().x < -4.0)
            {
                if (fleeing)
                {
                    adventurer_results.push_back({AdventurerResult::Fled, level, 0, 0.0f, loot / 80.0f, (level - courage + 1) * 1.1f});
                }
                else
                {
                    adventurer_results.push_back({AdventurerResult::Escaped, level, 0, 0.0f, loot / 80.0f, -courage - level * 0.2f});
                }
                delete this;
                return;
            }
        }
        if (hp < 1)
            delete this;
    }

    bool takeDamage(int amount)
    {
        hp -= amount;
        if (slimed)
            hp -= amount;
        if (hp > 0)
            return false;
        return true;
    }

    void addSlime()
    {
        slimed = true;
        render_data.color = sp::HsvColor(120, 70, 100);
    }

    bool addFear(int amount)
    {
        courage -= amount;
        if (slimed)
            courage -= amount;
        if (courage <= 0)
            fleeing = true;
        return fleeing;
    }

    int loot = 0;
    int level;
private:
    int hp = 1;
    int courage = 3;
    bool slimed = false;

    bool in_room = false;
    bool fleeing = false;
    sp::P<DungeonRoom> current_room;
    sp::PList<DungeonRoom> backtrack_list;
    std::unordered_set<DungeonRoom*> visited_rooms;
};

#include "effects.h"
#include "objects.h"

class AdventurerManager : public sp::Node
{
public:
    AdventurerManager(sp::P<sp::Node> parent)
    : sp::Node(parent)
    {
        spawn_count = 2;
        spawn_count += std::pow(reward, 0.5);
        spawn_count -= std::pow(risk, 0.3);
        spawn_count = std::min(10, spawn_count);
        spawn_count = std::max(2, spawn_count);
        max_level = std::max(1, int(1 + reward));
    }

    virtual void onFixedUpdate() override
    {
        if (spawn_count)
        {
            if (spawn_delay)
            {
                spawn_delay--;
            }
            else
            {
                adventurers.add(new Adventurer(getParent(), sp::irandom(1, max_level)));
                spawn_delay = sp::irandom(80, 140);
                spawn_count--;
            }
        }
        if (adventurers.size() == 0 && spawn_count == 0)
        {
            if (done_countdown)
                done_countdown--;
            else
                done = true;
        }
    }

    bool done = false;
private:
    int done_countdown = 100;
    int spawn_count = 5;
    int spawn_delay = 20;
    int max_level = 1;
    sp::PList<Adventurer> adventurers;
};

class DungeonScene : public sp::Scene
{
public:
    DungeonScene()
    : sp::Scene("DUNGEON")
    {
        main_ui.destroy();
        main_ui = sp::gui::Loader::load("gui/main.gui", "MAIN");

        sp::P<sp::Camera> camera = new sp::Camera(getRoot());
        setDefaultCamera(camera);
        camera->setOrtographic(sp::Vector2d(16, 16));
        camera->setPosition(sp::Vector2d(8, 0));

        DungeonRoom* dr;
        dr = new DungeonRoom(getRoot());
        dr->entrance = true;
        dr->doBuild();

        selection_indicator = new sp::Node(getRoot());
        selection_indicator->render_data.type = sp::RenderData::Type::None;
        selection_indicator->render_data.shader = sp::Shader::get("internal:color.shader");
        selection_indicator->render_data.mesh = sp::MeshData::createQuad(sp::Vector2f(4, 6));
        selection_indicator->render_data.color = sp::Color(0.15, 0.15, 0.15);
        selection_indicator->render_data.order = -1;

        updateUI();

        main_ui->getWidgetWithID("PLAY_BUTTON")->setEventCallback([this](sp::Variant v)
        {
            selected_room = nullptr;
            adventure_manager = new AdventurerManager(getRoot());
            updateUI();
        });
        main_ui->getWidgetWithID("DIG")->setEventCallback([this](sp::Variant v)
        {
            action = "DIG";
            updateUI();
        });
        main_ui->getWidgetWithID("PIT")->setEventCallback([this](sp::Variant v)
        {
            action = "PIT";
            updateUI();
        });
        main_ui->getWidgetWithID("LOOT")->setEventCallback([this](sp::Variant v)
        {
            action = "LOOT";
            updateUI();
        });
        main_ui->getWidgetWithID("FIRE")->setEventCallback([this](sp::Variant v)
        {
            action = "FIRE";
            updateUI();
        });
        main_ui->getWidgetWithID("SLIME")->setEventCallback([this](sp::Variant v)
        {
            action = "SLIME";
            updateUI();
        });
        main_ui->getWidgetWithID("BODY")->setEventCallback([this](sp::Variant v)
        {
            action = "BODY";
            updateUI();
        });
        main_ui->getWidgetWithID("SELL")->setEventCallback([this](sp::Variant v)
        {
            action = "SELL";
            updateUI();
        });
        main_ui->getWidgetWithID("BUILD_BUTTON")->setEventCallback([this](sp::Variant v)
        {
            if (cost_map.find(action) != cost_map.end())
            {
                if (money < cost_map[action])
                    return;
                money -= cost_map[action];
            }
            if (action == "DIG" && selected_room)
                selected_room->doBuild();
            else if (action == "PIT" && selected_room && !selected_room->main_object)
                selected_room->main_object = new SpikeTrap(selected_room);
            else if (action == "LOOT" && selected_room && !selected_room->main_object)
                selected_room->main_object = new Loot(selected_room);
            else if (action == "FIRE" && selected_room && !selected_room->main_object)
                selected_room->main_object = new FireTrap(selected_room);
            else if (action == "SLIME" && selected_room && !selected_room->main_object)
                selected_room->main_object = new Slime(selected_room);
            else if (action == "BODY" && selected_room && !selected_room->main_object)
                selected_room->main_object = new Body(selected_room);
            else if (action == "SELL" && selected_room && selected_room->main_object)
            {
                money += selected_room->main_object->value;
                selected_room->main_object.destroy();
            }
            action = "";
            updateUI();
        });
        main_ui->getWidgetWithID("RESULT_DONE_BUTTON")->setEventCallback([this](sp::Variant v)
        {
            main_ui->getWidgetWithID("RESULT_PANEL")->hide();
        });
    }

    virtual void onFixedUpdate() override
    {
        if (adventure_manager && adventure_manager->done)
        {
            for(sp::P<DungeonRoom> room : getRoot()->getChildren())
            {
                if (room)
                {
                    for(sp::P<DungeonObject> obj : room->getChildren())
                    {
                        if (obj)
                            obj->onEndOfDay();
                    }
                }
            }
            main_ui->getWidgetWithID("RESULT_PANEL")->show();
            while (!main_ui->getWidgetWithID("RESULT_PANEL")->getWidgetWithID("RESULT_ROWS")->getChildren().empty())
                (*main_ui->getWidgetWithID("RESULT_PANEL")->getWidgetWithID("RESULT_ROWS")->getChildren().begin()).destroy();
            risk *= 0.95f;
            reward *= 0.95f;
            dragon_deception *= 0.95f;
            for(auto& result : adventurer_results)
            {
                auto row = sp::gui::Loader::load("gui/main.gui", "RESULT_LINE", main_ui->getWidgetWithID("RESULT_PANEL")->getWidgetWithID("RESULT_ROWS"));
                switch(result.result)
                {
                case AdventurerResult::Death:
                    row->getWidgetWithID("RESULT")->setAttribute("caption", "Death");
                    break;
                case AdventurerResult::Escaped:
                    row->getWidgetWithID("RESULT")->setAttribute("caption", "Escaped");
                    break;
                case AdventurerResult::Fled:
                    row->getWidgetWithID("RESULT")->setAttribute("caption", "Fled");
                    break;
                }
                std::vector<sp::string> info;
                if (result.money > 0)
                {
                    money += result.money;
                    info.push_back("$" + sp::string(result.money));
                }
                risk += result.risk;
                if (result.risk > 2)
                    info.push_back("Risk++");
                else if (result.risk < -2)
                    info.push_back("Risk--");
                else if (result.risk > 0)
                    info.push_back("Risk+");
                else if (result.risk < 0)
                    info.push_back("Risk-");
                reward += result.reward;
                if (result.reward > 2)
                    info.push_back("Reward++");
                else if (result.reward < -2)
                    info.push_back("Reward--");
                else if (result.reward > 0)
                    info.push_back("Reward+");
                else if (result.reward < 0)
                    info.push_back("Reward-");
                dragon_deception += result.deception;
                if (result.deception > 2)
                    info.push_back("Deception++");
                else if (result.deception < -2)
                    info.push_back("Deception--");
                else if (result.deception > 0)
                    info.push_back("Deception+");
                else if (result.deception < 0)
                    info.push_back("Deception-");
                row->getWidgetWithID("INFO")->setAttribute("caption", sp::string(" ").join(info));
            }
            adventurer_results.clear();
            adventure_manager.destroy();
            risk = std::max(0.0f, risk);
            reward = std::max(0.0f, reward);
            dragon_deception = std::max(0.0f, dragon_deception);
            money += dragon_deception;
            main_ui->getWidgetWithID("RESULT_PANEL")->getWidgetWithID("TRIBUTE")->setAttribute("caption", "Tribute from villages: $" + sp::string(int(dragon_deception)));
            updateUI();
        }
    }

    virtual bool onPointerDown(sp::io::Pointer::Button button, sp::Ray3d ray, int id) override
    {
        return true;
    }

    virtual void onPointerDrag(sp::Ray3d ray, int id) override
    {
    }

    virtual void onPointerUp(sp::Ray3d ray, int id) override
    {
        if (adventure_manager)
            return;

        selected_room = getRoomAt(sp::Vector2d(ray.start.x, ray.start.y), true);
        action = "";
        updateUI();
    }

    virtual void onTextInput(const sp::string& text) override
    {
    }

    void updateUI()
    {
        if (adventure_manager)
        {
            selection_indicator->render_data.type = sp::RenderData::Type::None;
            main_ui->getWidgetWithID("BUILD_PANEL")->hide();
            main_ui->getWidgetWithID("INFO_PANEL")->hide();
        }
        else if (selected_room)
        {
            selection_indicator->setPosition(selected_room->getPosition2D());
            selection_indicator->render_data.type = sp::RenderData::Type::Normal;
            main_ui->getWidgetWithID("INFO_PANEL")->show();
            main_ui->getWidgetWithID("BUILD_PANEL")->show();
            main_ui->getWidgetWithID("DIG")->setVisible(!selected_room->build);
            main_ui->getWidgetWithID("PIT")->setVisible(selected_room->build && !selected_room->main_object);
            main_ui->getWidgetWithID("LOOT")->setVisible(selected_room->build && !selected_room->main_object);
            main_ui->getWidgetWithID("FIRE")->setVisible(selected_room->build && !selected_room->main_object);
            main_ui->getWidgetWithID("SLIME")->setVisible(selected_room->build && !selected_room->main_object);
            main_ui->getWidgetWithID("BODY")->setVisible(selected_room->build && !selected_room->main_object && placable_bodies > 0);
            main_ui->getWidgetWithID("SELL")->setVisible(selected_room->build && selected_room->main_object && selected_room->main_object->value > 0);

            main_ui->getWidgetWithID("BUILD_BUTTON")->setVisible(action != "");
            main_ui->getWidgetWithID("BUILD_BUTTON")->setAttribute("caption", action != "SELL" ? "[BUILD]" : "[SELL]");
        }
        else
        {
            selection_indicator->render_data.type = sp::RenderData::Type::None;
            main_ui->getWidgetWithID("INFO_PANEL")->show();
            main_ui->getWidgetWithID("BUILD_PANEL")->hide();
            main_ui->getWidgetWithID("BUILD_BUTTON")->hide();
        }

        sp::string info = "Money: " + sp::string(money);
        if (action == "DIG")
            info += "\nDig a new room.\nExpand your dungeon.";
        else if (action == "PIT")
            info += "\nBuild a spike pit\nSimple device,\nbut effective.";
        else if (action == "LOOT")
            info += "\nPlace a pile of gold\nto find.\nIf someone escapes\nwith this.\nIt will bring\nMore and better\nadventurers.";
        else if (action == "FIRE")
            info += "\nFire trap, burns\nadventurers.\nAdds a lot of\ndeception\non a kill.";
        else if (action == "SLIME")
            info += "\nSlime trap.\nMade with powered\ninsta-slime.\nIncreases damage\nand fear from other\ntraps.";
        else if (action == "BODY")
            info += "\nPlace a dead body.\nDecays after a\nfew days.\nAdds fear.\nAmount:" + sp::string(placable_bodies);
        else if (action == "SELL")
            info += "\nSell back for: $" + sp::string(selected_room->main_object->value);
        if (cost_map.find(action) != cost_map.end())
        {
            info += "\nCost: $" + sp::string(cost_map[action]);
        }
        main_ui->getWidgetWithID("INFO_LABEL")->setAttribute("caption", info);
    }

    sp::P<DungeonRoom> selected_room;
    sp::P<sp::Node> selection_indicator;

    sp::P<AdventurerManager> adventure_manager;
    sp::string action;
};

int main(int argc, char** argv)
{
    sp::P<sp::Engine> engine = new sp::Engine();

    //Create resource providers, so we can load things.
    new sp::io::DirectoryResourceProvider("resources");

    //Disable or enable smooth filtering by default, enabling it gives nice smooth looks, but disabling it gives a more pixel art look.
    sp::texture_manager.setDefaultSmoothFiltering(true);

    //Create a window to render on, and our engine.
    window = new sp::Window(4.0/3.0);
#ifndef DEBUG
//    window->setFullScreen(true);
//    window->hideCursor();
#endif

    sp::gui::Theme::loadTheme("default", "gui/theme/basic.theme.txt");
    new sp::gui::Scene(sp::Vector2d(640, 480));
    main_font = sp::font_manager.get("gui/theme/NanumGothicCoding-Bold.ttf");

    sp::P<sp::SceneGraphicsLayer> scene_layer = new sp::SceneGraphicsLayer(1);
    scene_layer->addRenderPass(new sp::BasicNodeRenderPass());
#ifdef DEBUG
    scene_layer->addRenderPass(new sp::CollisionRenderPass());
#endif
    window->addLayer(scene_layer);

    main_ui = sp::gui::Loader::load("gui/menu.gui", "MENU", nullptr, true);
    main_ui->getWidgetWithID("PLAY_BUTTON")->setEventCallback([](sp::Variant v)
    {
        new DungeonScene();
    });

    engine->run();

    return 0;
}
