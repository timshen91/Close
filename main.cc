#include <ctime>
#include <cstdlib>
#include <memory>
#include <vector>
#include <string>
#include <unistd.h>
#include <OgreRoot.h>
#include <OgreCamera.h>
#include <OgreSceneManager.h>
#include <OgreRenderWindow.h>
#include <OgreLogManager.h>
#include <OgreViewport.h>
#include <OgreConfigFile.h>
#include <OgreEntity.h>
#include <OgreWindowEventUtilities.h>
#include <Overlay/OgreOverlaySystem.h>
#include <Overlay/OgreOverlayManager.h>
#include <Overlay/OgreOverlayElement.h>
#include <Overlay/OgreOverlayContainer.h>

class MusicRunner {
    constexpr static double INTERVAL = 1000;

    double fake_timer;

public:
    constexpr static double BLOCK_SPEED = 0.5;

    MusicRunner() : fake_timer(0) {}

    int addTime(double delta_time) {
        fake_timer += delta_time;
        int count = 0;
        while (fake_timer >= INTERVAL) {
            fake_timer -= INTERVAL;
            count++;
        }
        return count;
    }
};

class StringIdAllocator {
    std::vector<Ogre::String> v;
    Ogre::String prefix;
    int count;

public:
    StringIdAllocator(const Ogre::String& prefix) : prefix(prefix), count(0) {}

    const Ogre::String& alloc() {
        v.push_back(prefix + std::to_string(count++));
        return v.back();
    }
};

class Main {
    MusicRunner runner;
    StringIdAllocator strAlloc;
    std::vector<Ogre::OverlayContainer*> blocks;
    std::unique_ptr<Ogre::Root> mRoot;
    std::unique_ptr<Ogre::OverlaySystem> mOverlaySystem;
    Ogre::RenderWindow* mWindow;
    Ogre::SceneManager* mSceneMgr;
    Ogre::Camera* mCamera;

    constexpr static int FPS_LIMIT = 60;

public:
    Main(Ogre::String mResourcesCfg = "resources.cfg", Ogre::String mPluginsCfg = "plugins.cfg") : strAlloc("Block:"), mRoot(std::make_unique<Ogre::Root>(mPluginsCfg)), mOverlaySystem(std::make_unique<Ogre::OverlaySystem>()) {
        assert(mRoot->restoreConfig() || mRoot->showConfigDialog());
        assert(mWindow = mRoot->initialise(true, "Main"));
        assert(mSceneMgr = mRoot->createSceneManager(Ogre::ST_GENERIC));
        assert(mCamera = mSceneMgr->createCamera("Cam"));

        initConfig(mResourcesCfg);
        Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

        std::cerr << "TIM: Material List:\n";
        for (auto it = Ogre::MaterialManager::getSingleton().getResourceIterator(); it.hasMoreElements(); it.moveNext()) {
            std::cerr << it.peekNextValue()->getName() << "\n";
        }

        std::cerr << "TIM: Font List:\n";
        for (auto it = Ogre::FontManager::getSingleton().getResourceIterator(); it.hasMoreElements(); it.moveNext()) {
            std::cerr << it.peekNextValue()->getName() << "\n";
        }

        Ogre::FontManager::getSingleton().getByName("SdkTrays/Caption")->load();

        mSceneMgr->addRenderQueueListener(mOverlaySystem.get());
        auto& overlayManager = Ogre::OverlayManager::getSingleton();
        auto overlay = overlayManager.getByName("MusicBox");
        overlay->show();
    }

    void go() {
        Ogre::Viewport* vp = mWindow->addViewport(mCamera);
        vp->setBackgroundColour(Ogre::ColourValue(0,0,0));

        //mCamera->setAspectRatio(Ogre::Real(vp->getActualWidth()) / Ogre::Real(vp->getActualHeight()));
        //mCamera->setPosition(Ogre::Vector3(0,0,80));
        //mCamera->lookAt(Ogre::Vector3(0,0,-300));
        //mCamera->setNearClipDistance(5);

        //Ogre::Entity* ogreHead = mSceneMgr->createEntity("Head", "ogrehead.mesh");

        //Ogre::SceneNode* headNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
        //headNode->attachObject(ogreHead);

        //mSceneMgr->setAmbientLight(Ogre::ColourValue(0.5, 0.5, 0.5));

        //Ogre::Light* l = mSceneMgr->createLight("MainLight");
        //l->setPosition(20,80,50);

        timeval tvb, tve, tv_last;
        struct timezone tz;
        gettimeofday(&tv_last, &tz);
        timeval fps_start, fps_end;
        gettimeofday(&fps_start, &tz);
        int fcount = 0;
        while (1) {
            bool tick = false;
            if (++fcount % (FPS_LIMIT * 10) == 0) {
                tick = true;
                gettimeofday(&fps_end, &tz);
                std::cerr << fcount / timeDiff(fps_start, fps_end) * 1000 << "\n";
                fcount = 0;
                gettimeofday(&fps_start, &tz);
            }
            gettimeofday(&tvb, &tz);
            Ogre::WindowEventUtilities::messagePump();
            if (mWindow->isClosed()) {
                return;
            }
            if (!mRoot->renderOneFrame()) {
                return;
            }
            oneFrame(timeDiff(tv_last, tvb), tick);
            auto tbegin = gettimeofday(&tve, &tz);
            double sleep_time = 1000 / FPS_LIMIT - timeDiff(tvb, tve);
            if (sleep_time > 0) {
                usleep((useconds_t)(sleep_time * 1000));
            }
            tv_last = tvb;
        }
    }

private:
    void initConfig(Ogre::String mResourcesCfg) {
        Ogre::ConfigFile cf;
        cf.load(mResourcesCfg);

        Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();

        Ogre::String secName, typeName, archName;
        while (seci.hasMoreElements()) {
            secName = seci.peekNextKey();
            Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();
            Ogre::ConfigFile::SettingsMultiMap::iterator i;
            for (i = settings->begin(); i != settings->end(); ++i) {
                typeName = i->first;
                archName = i->second;
                Ogre::ResourceGroupManager::getSingleton().addResourceLocation(archName, typeName, secName);
            }
        }
    }

    double timeDiff(const timeval& tvb, const timeval& tve) {
        return (tve.tv_sec - tvb.tv_sec) * 1000 + (tve.tv_usec - tvb.tv_usec) / 1000;
    }

    void oneFrame(double delta_time, bool tick) {
        std::vector<Ogre::OverlayContainer*> new_vec;
        for (auto it : blocks) {
            it->setTop(it->getTop() + MusicRunner::BLOCK_SPEED * delta_time / 1000);
            if (it->getTop() < 1) {
                new_vec.push_back(it);
            }
        }
        blocks = std::move(new_vec);
        if (tick) {
            std::cerr << blocks.size() << "\n";
        }
        int count = runner.addTime(delta_time);
        while (count--) {
            int new_block_idx = rand() % 4;

            auto& overlayManager = Ogre::OverlayManager::getSingleton();
            auto block = static_cast<Ogre::OverlayContainer*>(overlayManager.createOverlayElement("Panel", strAlloc.alloc()));
            static_cast<Ogre::OverlayContainer*>(overlayManager.getOverlayElement("MyOverlayElements/Main"))->addChild(block);
            block->setMaterialName("Sinbad/Ruby");
            block->setDimensions(0.125, 0.05);
            block->setLeft(new_block_idx * 0.125);
            block->setTop(0.0);
            blocks.push_back(block);
        }
    }
};

int main() {
    Main app;
    try {
        app.go();
    } catch(Ogre::Exception& e) {
        std::cerr << "An exception has occured: " << e.getFullDescription().c_str() << std::endl;
    }
    return 0;
}
