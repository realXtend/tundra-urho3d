// For conditions of distribution and use, see copyright notice in LICENSE

#include "TestRunner.h"
#include "TestBenchmark.h"

#include "Scene.h"
#include "Entity.h"
#include "LoggingFunctions.h"

#include <Engine/IO/FileSystem.h>

#include <kNet/DataSerializer.h>

using namespace Tundra;
using namespace Tundra::Test;

TEST_F(Runner, CreateEntity)
{
    foreach_std(bool replicated, TrueAndFalse)
    {
        foreach_std(bool temporary, TrueAndFalse)
        {
            Tundra::Benchmark::Iterations = 10000;

            BENCHMARK(PadString(replicated ? "Replicated" : "Local", 10) + PadString(temporary ? " + Temp" : "", 4), 25)
            {
                EntityPtr ent = scene->CreateEntity(0, StringVector(), AttributeChange::Default,
                    replicated, replicated, temporary);

                ASSERT_TRUE(ent != nullptr);
                ASSERT_TRUE(ent->ParentScene() != nullptr);
                ASSERT_TRUE(ent->Parent() == nullptr);

                ASSERT_EQ(ent->IsReplicated(), replicated);
                ASSERT_EQ(ent->IsUnacked(), !scene->IsAuthority());
                ASSERT_EQ(ent->IsTemporary(), temporary);

                ASSERT_EQ(ent->Name(), String::EMPTY);
                ASSERT_EQ(ent->Description(), String::EMPTY);
                ASSERT_EQ(ent->Group(), String::EMPTY);

                ASSERT_EQ(ent->NumComponents(), 0U);
                ASSERT_EQ(ent->NumChildren(), 0U);

                BENCHMARK_STEP_END;

                scene->RemoveEntity(ent->Id());
            }
            BENCHMARK_END;
        }
    }
}

TEST_F(Runner, CreateAttributes)
{
    kNet::DataSerializer dsName(64 * 1024);
    kNet::DataSerializer dsId(64 * 1024);

    StringVector types = SceneAPI::AttributeTypes();
    // Also test that the deprecated Q-prefixed type names work too.
    types.Push("QVariant");
    types.Push("QVariantList");
    types.Push("QPoint");
    foreach(const String &attributeTypeName, types)
    {
        u32 attributeTypeId = SceneAPI::AttributeTypeIdForTypeName(attributeTypeName);

        Tundra::Benchmark::Iterations = 10000;

        BENCHMARK(PadString(attributeTypeId, 3) + attributeTypeName, 25)
        {
            IAttribute *byName = SceneAPI::CreateAttribute(attributeTypeName, "ByName");

            ASSERT_TRUE(byName != nullptr);
            ASSERT_TRUE(byName->Owner() == nullptr);

            dsName.ResetFill();
            byName->ToBinary(dsName);

            ASSERT_TRUE(dsName.BytesFilled() > 0);

            IAttribute *byId = SceneAPI::CreateAttribute(attributeTypeId, "ById");

            ASSERT_TRUE(byId != nullptr);
            ASSERT_TRUE(byId->Owner() == nullptr);

            dsId.ResetFill();
            byName->ToBinary(dsId);

            ASSERT_TRUE(dsName.BytesFilled() > 0);

            ASSERT_EQ(dsName.BytesFilled(), dsId.BytesFilled());

            BENCHMARK_STEP_END;

            SAFE_DELETE(byName);
            SAFE_DELETE(byId);
        }
        BENCHMARK_END;
    }
}

TEST_F(Runner, CreateComponentsUnparented)
{
    StringVector types = framework->Scene()->ComponentTypes();

    foreach(const String &componentTypeName, types)
    {
        u32 componentTypeId = framework->Scene()->ComponentTypeIdForTypeName(componentTypeName);

        Tundra::Benchmark::Iterations = 10000;

        BENCHMARK(PadString(componentTypeId, 4) + componentTypeName, 25)
        {
            ASSERT_TRUE(framework->Scene()->IsComponentTypeRegistered(componentTypeName));
            ASSERT_TRUE(framework->Scene()->IsComponentFactoryRegistered(componentTypeName));

            ComponentPtr byName = framework->Scene()->CreateComponentByName(0, componentTypeName);

            ASSERT_TRUE(byName != nullptr);
            ASSERT_TRUE(byName->ParentScene() == nullptr);
            ASSERT_TRUE(byName->ParentEntity() == nullptr);

            ASSERT_EQ(byName->TypeId(), componentTypeId);
            ASSERT_EQ(byName->TypeName(), componentTypeName);

            ComponentPtr byId = framework->Scene()->CreateComponentById(0, componentTypeId);

            ASSERT_TRUE(byId != nullptr);
            ASSERT_TRUE(byId->ParentScene() == nullptr);
            ASSERT_TRUE(byId->ParentEntity() == nullptr);

            ASSERT_EQ(byId->TypeId(), componentTypeId);
            ASSERT_EQ(byId->TypeName(), componentTypeName);

            BENCHMARK_STEP_END;
        }
        BENCHMARK_END;
    }
}

TEST_F(Runner, CreateComponentsParented)
{
    StringVector types = framework->Scene()->ComponentTypes();
    foreach(const String &componentTypeName, types)
    {
        u32 componentTypeId = framework->Scene()->ComponentTypeIdForTypeName(componentTypeName);
        Log(componentTypeName + " " + String(componentTypeId), 1);

        foreach_std(bool replicated, TrueAndFalse)
        {
            foreach_std(bool temporary, TrueAndFalse)
            {
                Tundra::Benchmark::Iterations = 10000;

                BENCHMARK(PadString(replicated ? "Replicated" : "Local", 10) + PadString(temporary ? " + Temp" : "", 4), 25)
                {
                    EntityPtr parent = scene->CreateEntity(0, StringVector(), AttributeChange::Default,
                        replicated, replicated, temporary);

                    ComponentPtr byName = parent->CreateComponent(componentTypeName, "ByName", AttributeChange::Default, replicated);

                    ASSERT_TRUE(byName != nullptr);
                    ASSERT_TRUE(byName->ParentScene() != nullptr);
                    ASSERT_TRUE(byName->ParentEntity() != nullptr);

                    ASSERT_EQ(byName->ParentScene(), scene);
                    ASSERT_EQ(byName->ParentEntity(), parent);

                    ASSERT_EQ(byName->TypeId(), componentTypeId);
                    ASSERT_EQ(byName->TypeName(), componentTypeName);
                    ASSERT_EQ(byName->IsTemporary(), temporary);

                    ComponentPtr byId = parent->CreateComponent(componentTypeId, "ById", AttributeChange::Default, replicated);

                    ASSERT_TRUE(byId != nullptr);
                    ASSERT_TRUE(byId->ParentScene() != nullptr);
                    ASSERT_TRUE(byId->ParentEntity() != nullptr);

                    ASSERT_EQ(byId->ParentScene(), scene);
                    ASSERT_EQ(byId->ParentEntity(), parent);

                    ASSERT_EQ(byId->TypeId(), componentTypeId);
                    ASSERT_EQ(byId->TypeName(), componentTypeName);
                    ASSERT_EQ(byId->IsTemporary(), temporary);

                    BENCHMARK_STEP_END;

                    scene->RemoveEntity(parent->Id());
                }
                BENCHMARK_END;
            }
        }
    }
}

TEST_F(Runner, SceneSerialization)
{
    // Remove tundra.json hardcoded scene ents
    scene->RemoveAllEntities();

    String txmlPath = framework->GetSubsystem<Urho3D::FileSystem>()->GetProgramDir() + "TundraTestScene.txml";
    ASSERT_FALSE(txmlPath.Empty());

    StringVector types = framework->Scene()->ComponentTypes();
    foreach(const String &componentTypeName, types)
    {
        EntityPtr ent = scene->CreateEntity();
        ent->SetName("Entity_" + componentTypeName);
        ent->CreateComponent(componentTypeName, "Component_" + componentTypeName);
    }

    uint numEnts = scene->Entities().Size();
    ASSERT_EQ(types.Size(), numEnts);

    ASSERT_TRUE(scene->SaveSceneXML(txmlPath, false, false));

    scene->RemoveAllEntities();
    uint numEntsEmpty = scene->Entities().Size();

    Vector<Entity*> ents = scene->LoadSceneXML(txmlPath, true, true, AttributeChange::Default);

    // Cleanup file before any asserts can exit prematurely
    framework->GetSubsystem<Urho3D::FileSystem>()->Delete(txmlPath);

    ASSERT_EQ(numEntsEmpty, 0);
    ASSERT_EQ(ents.Size(), numEnts);

    foreach(const String &componentTypeName, types)
    {
        EntityPtr ent = scene->EntityByName("Entity_" + componentTypeName);
        ASSERT_TRUE(ent != nullptr);

        ComponentPtr comp = ent->Component(componentTypeName, "Component_" + componentTypeName);
        ASSERT_TRUE(comp != nullptr);

        Log(PadString(componentTypeName, 25) + "OK", 2);
    }
}

TUNDRA_TEST_MAIN();
