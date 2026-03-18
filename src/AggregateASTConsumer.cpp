#pragma once
#include "main.h"

class AggregateASTConsumer : public clang::ASTConsumer {
public:
    AggregateASTConsumer() {}

    void HandleTranslationUnit(clang::ASTContext& Ctx) override {
        for (auto consumer : consumers)
            consumer->HandleTranslationUnit(Ctx);
    }

    std::vector<ASTConsumer*> consumers;
};
