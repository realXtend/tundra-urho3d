// For conditions of distribution and use, see copyright notice in LICENSE

#include "TestRunner.h"
#include "TestBenchmark.h"

#include "Scene.h"
#include "Entity.h"

#include <kNet/DataSerializer.h>
#include <Engine/Container/ForEach.h>

using namespace Tundra;
using namespace Tundra::Test;

TEST_F(Runner, CreateEntity)
{
    for(size_t iRepl = 0; iRepl < NUMELEMS(TrueAndFalse); ++iRepl)
    {
        for(size_t iTemp = 0; iTemp < NUMELEMS(TrueAndFalse); ++iTemp)
        {
            const bool replicated = TrueAndFalse[iRepl];
            const bool temporary = TrueAndFalse[iTemp];

            Tundra::Benchmark::Iterations = 10000;

            BENCHMARK(PadString(replicated ? "Replicated" : "Local", 10) + PadString(temporary ? " + Temp" : "", 4), 25)
            {
                EntityPtr ent = scene->CreateEntity(0, StringVector(), AttributeChange::Default,
                    replicated, replicated, temporary);

                ASSERT_TRUE(ent != nullptr);
                ASSERT_TRUE(ent->ParentScene() != nullptr);
                ASSERT_TRUE(ent->Parent() == nullptr);

                ASSERT_EQ(ent->IsReplicated(), replicated);
                ASSERT_EQ(ent->IsUnacked(), replicated); // replicated == server needs to ack entity
                ASSERT_EQ(ent->IsTemporary(), temporary);

                ASSERT_EQ(ent->Name(), String::EMPTY);
                ASSERT_EQ(ent->Description(), String::EMPTY);
                ASSERT_EQ(ent->Group(), String::EMPTY);

                ASSERT_EQ(ent->NumComponents(), ZeroSizeT);
                ASSERT_EQ(ent->NumChildren(), ZeroSizeT);

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
    for (StringVector::ConstIterator iType = types.Begin(); iType != types.End(); ++iType)
    {
        String attributeTypeName = *iType;
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
    for (StringVector::ConstIterator iType = types.Begin(); iType != types.End(); ++iType)
    {
        String componentTypeName = *iType;
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
    for (StringVector::ConstIterator iType = types.Begin(); iType != types.End(); ++iType)
    {
        String componentTypeName = *iType;
        u32 componentTypeId = framework->Scene()->ComponentTypeIdForTypeName(componentTypeName);
        Log(componentTypeName + " " + String(componentTypeId), 1);

        for(size_t iRepl = 0; iRepl < NUMELEMS(TrueAndFalse); ++iRepl)
        {
            for(size_t iTemp = 0; iTemp < NUMELEMS(TrueAndFalse); ++iTemp)
            {
                const bool replicated = TrueAndFalse[iRepl];
                const bool temporary = TrueAndFalse[iTemp];

                Tundra::Benchmark::Iterations = 10000;

                BENCHMARK(PadString(replicated ? "Replicated" : "Local", 10) + PadString(temporary ? " + Temp" : "", 4), 25)
                {
                    EntityPtr parent = scene->CreateEntity(0, StringVector(), AttributeChange::Default,
                        replicated, replicated, temporary);

                    ComponentPtr byName = parent->CreateComponent(componentTypeName, "ByName", AttributeChange::Default, replicated);

                    ASSERT_TRUE(byName);
                    ASSERT_TRUE(byName->ParentScene());
                    ASSERT_TRUE(byName->ParentEntity());

                    ASSERT_EQ(byName->ParentScene(), scene);
                    ASSERT_EQ(byName->ParentEntity(), parent);

                    ASSERT_EQ(byName->TypeId(), componentTypeId);
                    ASSERT_EQ(byName->TypeName(), componentTypeName);
                    ASSERT_EQ(byName->IsTemporary(), temporary);

                    ComponentPtr byId = parent->CreateComponent(componentTypeId, "ById", AttributeChange::Default, replicated);

                    ASSERT_TRUE(byId);
                    ASSERT_TRUE(byId->ParentScene());
                    ASSERT_TRUE(byId->ParentEntity());

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

TUNDRA_TEST_MAIN();
