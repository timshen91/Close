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
#include <OIS.h>

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

class Main : public Ogre::FrameListener, OIS::KeyListener {
    MusicRunner runner;
    std::vector<Ogre::OverlayContainer*> blocks;
    std::unique_ptr<Ogre::Root> mRoot;
    std::unique_ptr<Ogre::OverlaySystem> mOverlaySystem;
    Ogre::RenderWindow* mWindow;
    Ogre::SceneManager* mSceneMgr;
    Ogre::Camera* mCamera;
    OIS::InputManager* mInputManager;
    OIS::Keyboard* mKeyboard;
    int idTop;
    bool isClosing;

    constexpr static int FPS_LIMIT = 60;

public:
    Main(Ogre::String mResourcesCfg = "resources.cfg", Ogre::String mPluginsCfg = "plugins.cfg") : mRoot(std::make_unique<Ogre::Root>(mPluginsCfg)), mOverlaySystem(std::make_unique<Ogre::OverlaySystem>()), idTop(0), isClosing(false) {
        assert(mRoot->restoreConfig() || mRoot->showConfigDialog());
        assert(mWindow = mRoot->initialise(true, "Main"));
        assert(mSceneMgr = mRoot->createSceneManager(Ogre::ST_GENERIC));
        assert(mCamera = mSceneMgr->createCamera("Cam"));

        OIS::ParamList pl;
        size_t windowHnd = 0;
        mWindow->getCustomAttribute("WINDOW", &windowHnd);
        pl.insert(std::make_pair(std::string("WINDOW"), std::to_string(windowHnd)));
        mInputManager = OIS::InputManager::createInputSystem(pl);
        mKeyboard = static_cast<OIS::Keyboard*>(mInputManager->createInputObject(OIS::OISKeyboard, true));

        registerListeners();

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
    }

    void go() {
        Ogre::FontManager::getSingleton().getByName("SdkTrays/Caption")->load();

        auto& overlayManager = Ogre::OverlayManager::getSingleton();
        overlayManager.getByName("MusicBox")->show();
        overlayManager.getByName("RunningBox")->show();

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

        mRoot->startRendering();

        //timeval tvb, tve, tv_last;
        //struct timezone tz;
        //gettimeofday(&tv_last, &tz);
        //timeval fps_start, fps_end;
        //gettimeofday(&fps_start, &tz);
        //int fcount = 0;
        //while (1) {
        //    bool tick = false;
        //    if (++fcount % (FPS_LIMIT * 10) == 0) {
        //        tick = true;
        //        gettimeofday(&fps_end, &tz);
        //        std::cerr << fcount / timeDiff(fps_start, fps_end) * 1000 << "\n";
        //        fcount = 0;
        //        gettimeofday(&fps_start, &tz);
        //    }
        //    gettimeofday(&tvb, &tz);
        //    Ogre::WindowEventUtilities::messagePump();
        //    if (mWindow->isClosed()) {
        //        return;
        //    }
        //    if (!mRoot->renderOneFrame()) {
        //        return;
        //    }
        //    oneFrame(timeDiff(tv_last, tvb), tick);
        //    auto tbegin = gettimeofday(&tve, &tz);
        //    double sleep_time = 1000 / FPS_LIMIT - timeDiff(tvb, tve);
        //    if (sleep_time > 0) {
        //        usleep((useconds_t)(sleep_time * 1000));
        //    }
        //    tv_last = tvb;
        //}
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

    void registerListeners() {
        mRoot->addFrameListener(this);
        mSceneMgr->addRenderQueueListener(mOverlaySystem.get());
        mKeyboard->setEventCallback(this);
    }

    static double timeDiff(const timeval& tvb, const timeval& tve) {
        return (tve.tv_sec - tvb.tv_sec) * 1000 + (tve.tv_usec - tvb.tv_usec) / 1000;
    }

    bool frameRenderingQueued(const Ogre::FrameEvent & evt) {
        if (mWindow->isClosed()) {
            return false;
        }
        timeval tvb, tve;
        struct timezone tz;
        gettimeofday(&tvb, &tz);

        auto delta_time = evt.timeSinceLastFrame * 1000;
        decltype(blocks) new_vec;
        for (auto it : blocks) {
            auto new_top = it->getTop() + MusicRunner::BLOCK_SPEED * delta_time / 1000;
            if (it->getTop() < 1) {
                it->setTop(new_top);
                new_vec.push_back(it);
            }
        }
        blocks = std::move(new_vec);
        int count = runner.addTime(delta_time);
        while (count--) {
            int new_block_idx = rand() % 4;

            auto& overlayManager = Ogre::OverlayManager::getSingleton();
            auto block = static_cast<Ogre::OverlayContainer*>(overlayManager.createOverlayElement("Panel", std::string("Block:") + std::to_string(idTop++)));
            static_cast<Ogre::OverlayContainer*>(overlayManager.getOverlayElement("RunningBox/Main"))->addChild(block);
            block->setMaterialName("SdkTrays/Logo");
            block->setDimensions(0.125, 0.05);
            block->setLeft(new_block_idx * 0.125);
            block->setTop(0.0);
            blocks.push_back(block);
        }
        mKeyboard->capture();

        gettimeofday(&tve, &tz);
        auto sleep_time = 1000.0 / FPS_LIMIT - timeDiff(tvb, tve) * 1000;
        if (sleep_time > 0) {
            usleep((useconds_t)(sleep_time * 1000));
        }
        return !isClosing;
    }

    bool keyPressed(const OIS::KeyEvent& arg) {
        int section;
        switch (arg.key) {
        case OIS::KC_H: section = 0; break;
        case OIS::KC_J: section = 1; break;
        case OIS::KC_K: section = 2; break;
        case OIS::KC_L: section = 3; break;
        case OIS::KC_ESCAPE: isClosing = true; break;
        default: return true;
        }
        return true;
    }

    bool keyReleased(const OIS::KeyEvent& arg) {
        return true;
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
