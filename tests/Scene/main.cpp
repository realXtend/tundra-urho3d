
#include "TestRunner.h"
#include "TestBenchmark.h"

#include "Scene.h"
#include "Entity.h"

#include "kNet/DataSerializer.h"

using namespace Tundra;
using namespace Tundra::Test;



TEST_F(Runner, CreateEntity)
{
    for (TrueFalseVec::ConstIterator iRepl = TrueFalse.Begin(); iRepl != TrueFalse.End(); ++iRepl)
    {
        for (TrueFalseVec::ConstIterator iTemp = TrueFalse.Begin(); iTemp != TrueFalse.End(); ++iTemp)
        {
            bool replicated = *iRepl;
            bool temporary = *iTemp;

            Tundra::Benchmark::Iterations = 10000;

            BENCHMARK(PadString(replicated ? "Replicated" : "Local", 10) + PadString(temporary ? " + Temp" : "", 4), 25)
            {
                EntityPtr ent = scene->CreateEntity(0, StringVector(), AttributeChange::Default,
                    replicated, replicated, temporary);

                EXPECT_TRUE(ent);
                EXPECT_TRUE(ent->ParentScene());
                EXPECT_TRUE(!ent->Parent());

                EXPECT_EQ(ent->IsReplicated(), replicated);
                EXPECT_EQ(ent->IsUnacked(), replicated); // replicated == server needs to ack entity
                EXPECT_EQ(ent->IsTemporary(), temporary);

                EXPECT_EQ(ent->Name(), EmptyString);
                EXPECT_EQ(ent->Description(), EmptyString);
                EXPECT_EQ(ent->Group(), EmptyString);

                EXPECT_EQ(ent->NumComponents(), ZeroSizeT);
                EXPECT_EQ(ent->NumChildren(), ZeroSizeT);

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

            EXPECT_TRUE(byName);
            EXPECT_TRUE(!byName->Owner());

            dsName.ResetFill();
            byName->ToBinary(dsName);

            EXPECT_TRUE(dsName.BytesFilled() > 0);

            IAttribute *byId = SceneAPI::CreateAttribute(attributeTypeId, "ById");

            EXPECT_TRUE(byId);
            EXPECT_TRUE(!byId->Owner());

            dsId.ResetFill();
            byName->ToBinary(dsId);

            EXPECT_TRUE(dsName.BytesFilled() > 0);

            EXPECT_EQ(dsName.BytesFilled(), dsId.BytesFilled());

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
            EXPECT_TRUE(framework->Scene()->IsComponentTypeRegistered(componentTypeName));
            EXPECT_TRUE(framework->Scene()->IsComponentFactoryRegistered(componentTypeName));

            ComponentPtr byName = framework->Scene()->CreateComponentByName(0, componentTypeName);

            EXPECT_TRUE(byName);
            EXPECT_TRUE(!byName->ParentScene());
            EXPECT_TRUE(!byName->ParentEntity());

            EXPECT_EQ(byName->TypeId(), componentTypeId);
            EXPECT_EQ(byName->TypeName(), componentTypeName);

            ComponentPtr byId = framework->Scene()->CreateComponentById(0, componentTypeId);

            EXPECT_TRUE(byId);
            EXPECT_TRUE(!byId->ParentScene());
            EXPECT_TRUE(!byId->ParentEntity());

            EXPECT_EQ(byId->TypeId(), componentTypeId);
            EXPECT_EQ(byId->TypeName(), componentTypeName);

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

        for (TrueFalseVec::ConstIterator iRepl = TrueFalse.Begin(); iRepl != TrueFalse.End(); ++iRepl)
        {
            for (TrueFalseVec::ConstIterator iTemp = TrueFalse.Begin(); iTemp != TrueFalse.End(); ++iTemp)
            {
                bool replicated = *iRepl;
                bool temporary = *iTemp;

                Tundra::Benchmark::Iterations = 10000;

                BENCHMARK(PadString(replicated ? "Replicated" : "Local", 10) + PadString(temporary ? " + Temp" : "", 4), 25)
                {
                    EntityPtr parent = scene->CreateEntity(0, StringVector(), AttributeChange::Default,
                        replicated, replicated, temporary);

                    ComponentPtr byName = parent->CreateComponent(componentTypeName, "ByName", AttributeChange::Default, replicated);

                    EXPECT_TRUE(byName);
                    EXPECT_TRUE(byName->ParentScene());
                    EXPECT_TRUE(byName->ParentEntity());

                    EXPECT_EQ(byName->ParentScene(), scene);
                    EXPECT_EQ(byName->ParentEntity(), parent);

                    EXPECT_EQ(byName->TypeId(), componentTypeId);
                    EXPECT_EQ(byName->TypeName(), componentTypeName);
                    EXPECT_EQ(byName->IsTemporary(), temporary);

                    ComponentPtr byId = parent->CreateComponent(componentTypeId, "ById", AttributeChange::Default, replicated);

                    EXPECT_TRUE(byId);
                    EXPECT_TRUE(byId->ParentScene());
                    EXPECT_TRUE(byId->ParentEntity());

                    EXPECT_EQ(byId->ParentScene(), scene);
                    EXPECT_EQ(byId->ParentEntity(), parent);

                    EXPECT_EQ(byId->TypeId(), componentTypeId);
                    EXPECT_EQ(byId->TypeName(), componentTypeName);
                    EXPECT_EQ(byId->IsTemporary(), temporary);

                    BENCHMARK_STEP_END;

                    scene->RemoveEntity(parent->Id());
                }
                BENCHMARK_END;
            }
        }
    }
}

TUNDRA_TEST_MAIN();
