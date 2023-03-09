//
// Created by manta on 11/10/22.
//

#ifndef CROSSOVER_BCBUILDER_H
#define CROSSOVER_BCBUILDER_H

#include <llvm/IR/IRBuilder.h>
#include "../../Crossover_bstd_lib//include/number.h"
#include "bcmodule.h"
#include "../../Crossover_bstd_lib/include/picture.h"
#include "../antlr/BabyCobolParser.h"

class BCBuilder : public llvm::IRBuilder<> {

protected:
    BCModule* module;

public:
    BCBuilder(BCModule* module, llvm::BasicBlock* bb) : llvm::IRBuilder<>(bb), module(module) {};

    /**
     * todo: doc
     * @return
     */
    llvm::Value* CreateNumber(BabyCobolParser::IntLiteralContext* context);

    /**
     * todo: doc
     * @return
     */
    llvm::Value* CreateNumber(BabyCobolParser::DoubleLiteralContext* context);

    /**
     * todo: doc
     * @param number
     * @return
     */
    llvm::Value* CreateNumber(bstd_number* number, std::string& name, bool global = false);

    /**
     * todo: doc
     * @param picture
     * @param name
     * @param global
     * @return
     */
    llvm::Value* CreatePicture(bstd_picture* picture, std::string &name, bool global = false);

    /**
     * todo: doc
     * @param lhs
     * @param rhs
     * @return
     */
    llvm::Value* CreateAdd(bstd_number* lhs, bstd_number* rhs);

    /**
     * todo: doc
     * @param n
     * @return 32-bit constant LLVM value representing the specified integer n.
     */
    llvm::Constant* asConstant(int n);

    /**
     * todo: doc
     * @return
     */
    llvm::Value *CreateNumberValue(const std::string& name, uint64_t m_value, uint64_t m_scale, uint8_t m_length, bool m_isSigned, bool m_isPositive, bool global);

    /**
     * Creates a call to the bstd runtime library marshaller function bstd_picture_to_cstr(bstd_picture*).
     * @param picture The picture to marshall.
     * @return Returns an pointer value referencing the c-style string representation of the specified picture.
     */
    llvm::Value* CreatePictureToCStrCall(llvm::Value* picture);

    /**
     * Creates a call to the bstd runtime library assignment function bstd_assign_cstr(bstd_picture*, char*).
     * @param picture The picture to assign the specified c-style string to.
     * @param str The c-style string to assign to the specified picture.
     * @return Returns void.
     */
    llvm::Value* CreateCStrToPictureCall(llvm::Value* picture, llvm::Value* str);

    /**
     * Creates a call to the bstd runtime library marshaller function bstd_number_to_int(Number*)
     * @param number The number to marshall
     * @return Returns an pointer value referencing the integer representation of the specified number.
     */
    llvm::Value* CreateNumberToIntPtrCall(llvm::Value* number);


    int LiteralCount = 0;
};

#endif //CROSSOVER_BCBUILDER_H
