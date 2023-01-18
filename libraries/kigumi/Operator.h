#pragma once

namespace kigumi {

// Face output table
//
//     |                  tag
//     |
//  op |   Uni.  |   Int.  |   Cop.  |   Opp.
// ----+---------+---------+---------+---------
//   V |         |         |         |
//   A |  A &  B |         |  A |  B |
//   B |      ~B |  A      |         |  A | ~B
//   C | ~A      |       B |         | ~A |  B
//   D |         | ~A & ~B | ~A | ~B |
//   E | ~A & ~B |  A &  B |         |
//   F | ~A      | ~A      | ~A      | ~A
//   G |      ~B |      ~B |      ~B |      ~B
//   H |       B |       B |       B |       B
//   I |  A      |  A      |  A      |  A
//   J |  A &  B | ~A & ~B |         |
//   K |         |  A &  B |  A |  B |
//   L |  A      |      ~B |         |  A | ~B
//   M |       B | ~A      |         | ~A |  B
//   X | ~A & ~B |         | ~A | ~B |
//   O |         |         |         |
//
// A & B: Output both A and B.
// A | B: Output either A or B.

enum class Operator {
  // Bocheński notation
  V,  // The universe
  A,  // A ∪ B
  B,  // (B ⧵ A)ᶜ
  C,  // (A ⧵ B)ᶜ
  D,  // (A ∩ B)ᶜ
  E,  // (A △ B)ᶜ
  F,  // Aᶜ
  G,  // Bᶜ
  H,  // B
  I,  // A
  J,  // A △ B
  K,  // A ∩ B
  L,  // A ⧵ B
  M,  // B ⧵ A
  X,  // (A ∪ B)ᶜ
  O,  // ∅
  Union = A,
  SymmetricDifference = J,
  Intersection = K,
  Difference = L,
};

enum class Mask {
  None = 0,
  A = 1,
  B = 2,
  AInv = 4,
  BInv = 8,
};

inline Mask operator|(Mask a, Mask b) {
  return static_cast<Mask>(static_cast<int>(a) | static_cast<int>(b));
}

inline Mask operator&(Mask a, Mask b) {
  return static_cast<Mask>(static_cast<int>(a) & static_cast<int>(b));
}

inline Mask union_mask(Operator op) {
  switch (op) {
    case Operator::I:
    case Operator::L:
      return Mask::A;
    case Operator::H:
    case Operator::M:
      return Mask::B;
    case Operator::A:
    case Operator::J:
      return Mask::A | Mask::B;
    case Operator::C:
    case Operator::F:
      return Mask::AInv;
    case Operator::B:
    case Operator::G:
      return Mask::BInv;
    case Operator::E:
    case Operator::X:
      return Mask::AInv | Mask::BInv;
    default:
      return Mask::None;
  }
}

inline Mask intersection_mask(Operator op) {
  switch (op) {
    case Operator::B:
    case Operator::I:
      return Mask::A;
    case Operator::C:
    case Operator::H:
      return Mask::B;
    case Operator::E:
    case Operator::K:
      return Mask::A | Mask::B;
    case Operator::F:
    case Operator::M:
      return Mask::AInv;
    case Operator::G:
    case Operator::L:
      return Mask::BInv;
    case Operator::D:
    case Operator::J:
      return Mask::AInv | Mask::BInv;
    default:
      return Mask::None;
  }
}

inline Mask coplanar_mask(Operator op, bool prefer_a) {
  switch (op) {
    case Operator::I:
      return Mask::A;
    case Operator::H:
      return Mask::B;
    case Operator::A:
    case Operator::K:
      return prefer_a ? Mask::A : Mask::B;
    case Operator::F:
      return Mask::AInv;
    case Operator::G:
      return Mask::BInv;
    case Operator::D:
    case Operator::X:
      return prefer_a ? Mask::AInv : Mask::BInv;
    default:
      return Mask::None;
  }
}

inline Mask opposite_mask(Operator op, bool prefer_a) {
  switch (op) {
    case Operator::I:
      return Mask::A;
    case Operator::H:
      return Mask::B;
    case Operator::B:
    case Operator::L:
      return prefer_a ? Mask::A : Mask::BInv;
    case Operator::F:
      return Mask::AInv;
    case Operator::G:
      return Mask::BInv;
    case Operator::C:
    case Operator::M:
      return prefer_a ? Mask::AInv : Mask::B;
    default:
      return Mask::None;
  }
}

}  // namespace kigumi
