// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "CoreTypes.h"
#include "CoreDefines.h"
#include "Framework.h"
#include "SceneAPI.h"
#include "Scene.h"
#include "CoreStringUtils.h"

#include <Math/MathFunc.h>

#include <Engine/Core/Context.h>
#include <Engine/Core/ProcessUtils.h>

#include <gtest/gtest.h>

#include <vector>

namespace Tundra
{
    namespace Test
    {
        /// An array containing true and false.
        static const bool TrueAndFalse[2] = { true, false };

        inline String TruthyString(bool truthy)
        {
            return truthy ? "true" : "false";
        }

        /// Typedefs for TestFramework
        typedef SharedPtr<Urho3D::Context> ContextPtr;
        typedef SharedPtr<Framework> FrameworkPtr;

        /** Unit test framework
            Helps out initialization and releasing framework/application ptrs correctly after the test.
            @note Put this object in your class (stack variable) and call Initialize once in your
            tests entry point: initTestCase(). */
        class Runner : public testing::Test
        {
        public:
            //static int argc;
            //static char *argv[];

            Runner(bool createScene = true, bool server = false, bool headless = true, std::string config = "")
            {
                this->createScene = createScene;
                this->server = server;
                this->headless = headless;
                this->config = config;
            }

            void Log(const String &msg, int pad = 0)
            {
                LogLine(msg, pad, false);
            }

            void LogError(const String &msg, int pad = 0)
            {
                LogLine(msg, pad, false);
            }

        protected:
            // Exposed to the fixtured test case
            ContextPtr context;
            FrameworkPtr framework;
            ScenePtr scene;

            void SetUp() override
            {
                arguments.push_back("TundraTestRunner.exe");

                // Let tests write log file for inspection on failures,
                // don't log anything to console.
                arguments.push_back("--quiet");

                if (server)
                    arguments.push_back("--server");
                if (headless)
                    arguments.push_back("--headless");
                if (!config.empty())
                {
                    arguments.push_back("--config");
                    arguments.push_back(config.c_str());
                }

                set_run_args((int)arguments.size(), (char**)&arguments[0]);

                context = new Urho3D::Context();
                framework = new Framework(context.Get());
                framework->Initialize();

                if (createScene)
                    scene = framework->Scene()->CreateScene("TestScene", false, true);

                ProcessEvents();
            }

            void TearDown() override
            {
                scene.Reset();

                framework->Uninitialize();

                // Explicit releasing order
                framework.Reset();
                context.Reset();

                arguments.clear();
                config.clear();
            }

            /// Call this function whenever you need Qt events, eg. signals, to be processed.
            void ProcessEvents()
            {
                framework->Pump();
            }

        private:
            void LogLine(const String &msg, int pad, bool error)
            {
                String padstr = "";
                while(pad > 0)
                {
                    padstr += " ";
                    pad--;
                }
                Urho3D::PrintLine(padstr + msg, error);
            }

            std::vector<const char*> arguments;

            std::string config;
            bool createScene;
            bool server;
            bool headless;
        };
    }
}

#define TUNDRA_TEST_MAIN() \
int main(int argc, char *argv[]) \
{ \
    testing::InitGoogleTest(&argc, argv); \
    /*Tundra::Test::Runner::argc = argc;*/ \
    /*Tundra::Test::Runner::argv = argv;*/ \
    return RUN_ALL_TESTS(); \
}
