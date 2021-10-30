#include <algorithm>
#include <climits>
#include "EternalModLoader.hpp"

// Horde Mode coin scoringItem decl with zero score
constexpr uint8_t HordeCoinScoreNullifier[] = {
    0x7B, 0x65, 0x64, 0x69, 0x74, 0x3D, 0x7B, 0x69, 0x63, 0x6F, 0x6E, 0x3D, 0x22, 0x61, 0x72, 0x74,
    0x2F, 0x75, 0x69, 0x2F, 0x69, 0x63, 0x6F, 0x6E, 0x73, 0x2F, 0x68, 0x6F, 0x72, 0x64, 0x65, 0x2F,
    0x69, 0x63, 0x6F, 0x6E, 0x5F, 0x62, 0x6F, 0x6E, 0x75, 0x73, 0x5F, 0x63, 0x6F, 0x69, 0x6E, 0x22,
    0x3B, 0x65, 0x76, 0x65, 0x6E, 0x74, 0x54, 0x79, 0x70, 0x65, 0x3D, 0x22, 0x53, 0x43, 0x4F, 0x52,
    0x45, 0x5F, 0x45, 0x56, 0x45, 0x4E, 0x54, 0x5F, 0x54, 0x59, 0x50, 0x45, 0x5F, 0x43, 0x4F, 0x49,
    0x4E, 0x22, 0x3B, 0x73, 0x74, 0x61, 0x74, 0x53, 0x63, 0x6F, 0x72, 0x65, 0x3D, 0x22, 0x53, 0x54,
    0x41, 0x54, 0x5F, 0x48, 0x4F, 0x52, 0x44, 0x45, 0x5F, 0x43, 0x4F, 0x49, 0x4E, 0x53, 0x5F, 0x47,
    0x41, 0x54, 0x48, 0x45, 0x52, 0x45, 0x44, 0x5F, 0x53, 0x43, 0x4F, 0x52, 0x45, 0x22, 0x3B, 0x7D,
    0x7D
};

// Horde Mode challenge complete scoringItem decl with zero score
constexpr uint8_t ChallengeCompleteScoreNullifier[] = {
    0x7B, 0x65, 0x64, 0x69, 0x74, 0x3D, 0x7B, 0x73, 0x63, 0x6F, 0x72, 0x65, 0x3D, 0x30, 0x3B, 0x69,
    0x63, 0x6F, 0x6E, 0x3D, 0x22, 0x61, 0x72, 0x74, 0x2F, 0x75, 0x69, 0x2F, 0x69, 0x63, 0x6F, 0x6E,
    0x73, 0x2F, 0x63, 0x6F, 0x64, 0x65, 0x78, 0x2F, 0x75, 0x61, 0x63, 0x22, 0x3B, 0x65, 0x76, 0x65,
    0x6E, 0x74, 0x54, 0x79, 0x70, 0x65, 0x3D, 0x22, 0x53, 0x43, 0x4F, 0x52, 0x45, 0x5F, 0x45, 0x56,
    0x45, 0x4E, 0x54, 0x5F, 0x54, 0x59, 0x50, 0x45, 0x5F, 0x43, 0x48, 0x41, 0x4C, 0x4C, 0x45, 0x4E,
    0x47, 0x45, 0x22, 0x3B, 0x7D, 0x7D
};

// Nullified Horde scoringRubric decl
constexpr uint8_t NullifiedScoringRubric[] = {
    0x7B, 0x65, 0x64, 0x69, 0x74, 0x3D, 0x7B, 0x64, 0x69, 0x66, 0x66, 0x69, 0x63, 0x75, 0x6C, 0x74,
    0x79, 0x53, 0x63, 0x6F, 0x72, 0x65, 0x42, 0x6F, 0x6E, 0x75, 0x73, 0x3D, 0x7B, 0x70, 0x74, 0x72,
    0x3D, 0x7B, 0x70, 0x74, 0x72, 0x5B, 0x30, 0x5D, 0x3D, 0x7B, 0x76, 0x61, 0x6C, 0x75, 0x65, 0x3D,
    0x30, 0x3B, 0x7D, 0x70, 0x74, 0x72, 0x5B, 0x31, 0x5D, 0x3D, 0x7B, 0x76, 0x61, 0x6C, 0x75, 0x65,
    0x3D, 0x30, 0x3B, 0x64, 0x69, 0x66, 0x66, 0x69, 0x63, 0x75, 0x6C, 0x74, 0x79, 0x3D, 0x22, 0x44,
    0x49, 0x46, 0x46, 0x49, 0x43, 0x55, 0x4C, 0x54, 0x59, 0x5F, 0x4D, 0x45, 0x44, 0x49, 0x55, 0x4D,
    0x22, 0x3B, 0x7D, 0x70, 0x74, 0x72, 0x5B, 0x32, 0x5D, 0x3D, 0x7B, 0x76, 0x61, 0x6C, 0x75, 0x65,
    0x3D, 0x30, 0x3B, 0x64, 0x69, 0x66, 0x66, 0x69, 0x63, 0x75, 0x6C, 0x74, 0x79, 0x3D, 0x22, 0x44,
    0x49, 0x46, 0x46, 0x49, 0x43, 0x55, 0x4C, 0x54, 0x59, 0x5F, 0x48, 0x41, 0x52, 0x44, 0x22, 0x3B,
    0x7D, 0x70, 0x74, 0x72, 0x5B, 0x33, 0x5D, 0x3D, 0x7B, 0x76, 0x61, 0x6C, 0x75, 0x65, 0x3D, 0x30,
    0x3B, 0x64, 0x69, 0x66, 0x66, 0x69, 0x63, 0x75, 0x6C, 0x74, 0x79, 0x3D, 0x22, 0x44, 0x49, 0x46,
    0x46, 0x49, 0x43, 0x55, 0x4C, 0x54, 0x59, 0x5F, 0x4E, 0x49, 0x47, 0x48, 0x54, 0x4D, 0x41, 0x52,
    0x45, 0x22, 0x3B, 0x7D, 0x70, 0x74, 0x72, 0x5B, 0x34, 0x5D, 0x3D, 0x7B, 0x64, 0x69, 0x66, 0x66,
    0x69, 0x63, 0x75, 0x6C, 0x74, 0x79, 0x3D, 0x22, 0x44, 0x49, 0x46, 0x46, 0x49, 0x43, 0x55, 0x4C,
    0x54, 0x59, 0x5F, 0x55, 0x4C, 0x54, 0x52, 0x41, 0x5F, 0x4E, 0x49, 0x47, 0x48, 0x54, 0x4D, 0x41,
    0x52, 0x45, 0x22, 0x3B, 0x7D, 0x7D, 0x7D, 0x64, 0x65, 0x6D, 0x6F, 0x6E, 0x49, 0x74, 0x65, 0x6D,
    0x73, 0x3D, 0x7B, 0x6E, 0x75, 0x6D, 0x3D, 0x30, 0x3B, 0x7D, 0x62, 0x6F, 0x75, 0x6E, 0x74, 0x79,
    0x49, 0x74, 0x65, 0x6D, 0x73, 0x3D, 0x7B, 0x6E, 0x75, 0x6D, 0x3D, 0x30, 0x3B, 0x7D, 0x76, 0x69,
    0x6F, 0x6C, 0x65, 0x6E, 0x63, 0x65, 0x45, 0x76, 0x65, 0x6E, 0x74, 0x49, 0x74, 0x65, 0x6D, 0x73,
    0x3D, 0x7B, 0x6E, 0x75, 0x6D, 0x3D, 0x30, 0x3B, 0x7D, 0x65, 0x76, 0x65, 0x6E, 0x74, 0x49, 0x74,
    0x65, 0x6D, 0x73, 0x3D, 0x7B, 0x6E, 0x75, 0x6D, 0x3D, 0x30, 0x3B, 0x7D, 0x65, 0x6E, 0x64, 0x4F,
    0x66, 0x4C, 0x65, 0x76, 0x65, 0x6C, 0x49, 0x74, 0x65, 0x6D, 0x73, 0x3D, 0x7B, 0x6E, 0x75, 0x6D,
    0x3D, 0x30, 0x3B, 0x7D, 0x7D, 0x7D
};

// Generic SWF data
constexpr uint8_t GenericSwfData[] = {
    0x25, 0x45, 0x70, 0x00, 0x00, 0x45, 0x07, 0x00, 0x00, 0x3C, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2B, 0x00, 0x00, 0x00, 0x2C, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00, 0x72, 0x6F, 0x6C, 0x6C, 0x4F,
    0x6E, 0x42, 0x61, 0x63, 0x6B, 0x00, 0x00, 0x00, 0x0D, 0x0C, 0x00, 0x00, 0x00, 0x72, 0x6F, 0x6C,
    0x6C, 0x4F, 0x66, 0x66, 0x46, 0x72, 0x6F, 0x6E, 0x74, 0x00, 0x00, 0x00, 0x17, 0x0B, 0x00, 0x00,
    0x00, 0x72, 0x6F, 0x6C, 0x6C, 0x4F, 0x6E, 0x46, 0x72, 0x6F, 0x6E, 0x74, 0x00, 0x00, 0x00, 0x21,
    0x0B, 0x00, 0x00, 0x00, 0x72, 0x6F, 0x6C, 0x6C, 0x4F, 0x66, 0x66, 0x42, 0x61, 0x63, 0x6B, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x00, 0x00, 0x0E, 0x26, 0x01, 0x00, 0x01, 0x00,
    0x00, 0x5F, 0x63, 0x65, 0x6E, 0x74, 0x65, 0x72, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00,
    0x00, 0x67, 0x65, 0x6E, 0x65, 0x72, 0x69, 0x63, 0x5F, 0x66, 0x6C, 0x61, 0x2E, 0x4D, 0x61, 0x69,
    0x6E, 0x54, 0x69, 0x6D, 0x65, 0x6C, 0x69, 0x6E, 0x65, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x04,
    0xBE, 0x01, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x2E, 0x00, 0x00, 0x00, 0x00, 0x34, 0x0B, 0x67,
    0x65, 0x6E, 0x65, 0x72, 0x69, 0x63, 0x5F, 0x66, 0x6C, 0x61, 0x0C, 0x4D, 0x61, 0x69, 0x6E, 0x54,
    0x69, 0x6D, 0x65, 0x6C, 0x69, 0x6E, 0x65, 0x0D, 0x66, 0x6C, 0x61, 0x73, 0x68, 0x2E, 0x64, 0x69,
    0x73, 0x70, 0x6C, 0x61, 0x79, 0x09, 0x4D, 0x6F, 0x76, 0x69, 0x65, 0x43, 0x6C, 0x69, 0x70, 0x18,
    0x67, 0x65, 0x6E, 0x65, 0x72, 0x69, 0x63, 0x5F, 0x66, 0x6C, 0x61, 0x3A, 0x4D, 0x61, 0x69, 0x6E,
    0x54, 0x69, 0x6D, 0x65, 0x6C, 0x69, 0x6E, 0x65, 0x00, 0x07, 0x5F, 0x63, 0x65, 0x6E, 0x74, 0x65,
    0x72, 0x06, 0x66, 0x72, 0x61, 0x6D, 0x65, 0x31, 0x07, 0x66, 0x72, 0x61, 0x6D, 0x65, 0x31, 0x32,
    0x07, 0x66, 0x72, 0x61, 0x6D, 0x65, 0x32, 0x32, 0x07, 0x66, 0x72, 0x61, 0x6D, 0x65, 0x33, 0x32,
    0x07, 0x66, 0x72, 0x61, 0x6D, 0x65, 0x34, 0x33, 0x07, 0x76, 0x69, 0x73, 0x69, 0x62, 0x6C, 0x65,
    0x21, 0x68, 0x74, 0x74, 0x70, 0x3A, 0x2F, 0x2F, 0x61, 0x64, 0x6F, 0x62, 0x65, 0x2E, 0x63, 0x6F,
    0x6D, 0x2F, 0x41, 0x53, 0x33, 0x2F, 0x32, 0x30, 0x30, 0x36, 0x2F, 0x62, 0x75, 0x69, 0x6C, 0x74,
    0x69, 0x6E, 0x0B, 0x61, 0x64, 0x6F, 0x62, 0x65, 0x2E, 0x75, 0x74, 0x69, 0x6C, 0x73, 0x13, 0x66,
    0x6C, 0x61, 0x73, 0x68, 0x2E, 0x61, 0x63, 0x63, 0x65, 0x73, 0x73, 0x69, 0x62, 0x69, 0x6C, 0x69,
    0x74, 0x79, 0x0D, 0x66, 0x6C, 0x61, 0x73, 0x68, 0x2E, 0x64, 0x65, 0x73, 0x6B, 0x74, 0x6F, 0x70,
    0x0C, 0x66, 0x6C, 0x61, 0x73, 0x68, 0x2E, 0x65, 0x72, 0x72, 0x6F, 0x72, 0x73, 0x0C, 0x66, 0x6C,
    0x61, 0x73, 0x68, 0x2E, 0x65, 0x76, 0x65, 0x6E, 0x74, 0x73, 0x0E, 0x66, 0x6C, 0x61, 0x73, 0x68,
    0x2E, 0x65, 0x78, 0x74, 0x65, 0x72, 0x6E, 0x61, 0x6C, 0x0D, 0x66, 0x6C, 0x61, 0x73, 0x68, 0x2E,
    0x66, 0x69, 0x6C, 0x74, 0x65, 0x72, 0x73, 0x0A, 0x66, 0x6C, 0x61, 0x73, 0x68, 0x2E, 0x67, 0x65,
    0x6F, 0x6D, 0x13, 0x66, 0x6C, 0x61, 0x73, 0x68, 0x2E, 0x67, 0x6C, 0x6F, 0x62, 0x61, 0x6C, 0x69,
    0x7A, 0x61, 0x74, 0x69, 0x6F, 0x6E, 0x0B, 0x66, 0x6C, 0x61, 0x73, 0x68, 0x2E, 0x6D, 0x65, 0x64,
    0x69, 0x61, 0x09, 0x66, 0x6C, 0x61, 0x73, 0x68, 0x2E, 0x6E, 0x65, 0x74, 0x0D, 0x66, 0x6C, 0x61,
    0x73, 0x68, 0x2E, 0x6E, 0x65, 0x74, 0x2E, 0x64, 0x72, 0x6D, 0x0E, 0x66, 0x6C, 0x61, 0x73, 0x68,
    0x2E, 0x70, 0x72, 0x69, 0x6E, 0x74, 0x69, 0x6E, 0x67, 0x0E, 0x66, 0x6C, 0x61, 0x73, 0x68, 0x2E,
    0x70, 0x72, 0x6F, 0x66, 0x69, 0x6C, 0x65, 0x72, 0x0D, 0x66, 0x6C, 0x61, 0x73, 0x68, 0x2E, 0x73,
    0x61, 0x6D, 0x70, 0x6C, 0x65, 0x72, 0x0D, 0x66, 0x6C, 0x61, 0x73, 0x68, 0x2E, 0x73, 0x65, 0x6E,
    0x73, 0x6F, 0x72, 0x73, 0x0C, 0x66, 0x6C, 0x61, 0x73, 0x68, 0x2E, 0x73, 0x79, 0x73, 0x74, 0x65,
    0x6D, 0x0A, 0x66, 0x6C, 0x61, 0x73, 0x68, 0x2E, 0x74, 0x65, 0x78, 0x74, 0x0E, 0x66, 0x6C, 0x61,
    0x73, 0x68, 0x2E, 0x74, 0x65, 0x78, 0x74, 0x2E, 0x69, 0x6D, 0x65, 0x11, 0x66, 0x6C, 0x61, 0x73,
    0x68, 0x2E, 0x74, 0x65, 0x78, 0x74, 0x2E, 0x65, 0x6E, 0x67, 0x69, 0x6E, 0x65, 0x08, 0x66, 0x6C,
    0x61, 0x73, 0x68, 0x2E, 0x75, 0x69, 0x0B, 0x66, 0x6C, 0x61, 0x73, 0x68, 0x2E, 0x75, 0x74, 0x69,
    0x6C, 0x73, 0x09, 0x66, 0x6C, 0x61, 0x73, 0x68, 0x2E, 0x78, 0x6D, 0x6C, 0x17, 0x66, 0x6C, 0x61,
    0x73, 0x68, 0x2E, 0x64, 0x69, 0x73, 0x70, 0x6C, 0x61, 0x79, 0x3A, 0x4D, 0x6F, 0x76, 0x69, 0x65,
    0x43, 0x6C, 0x69, 0x70, 0x14, 0x66, 0x6C, 0x61, 0x73, 0x68, 0x2E, 0x64, 0x69, 0x73, 0x70, 0x6C,
    0x61, 0x79, 0x3A, 0x53, 0x70, 0x72, 0x69, 0x74, 0x65, 0x24, 0x66, 0x6C, 0x61, 0x73, 0x68, 0x2E,
    0x64, 0x69, 0x73, 0x70, 0x6C, 0x61, 0x79, 0x3A, 0x44, 0x69, 0x73, 0x70, 0x6C, 0x61, 0x79, 0x4F,
    0x62, 0x6A, 0x65, 0x63, 0x74, 0x43, 0x6F, 0x6E, 0x74, 0x61, 0x69, 0x6E, 0x65, 0x72, 0x1F, 0x66,
    0x6C, 0x61, 0x73, 0x68, 0x2E, 0x64, 0x69, 0x73, 0x70, 0x6C, 0x61, 0x79, 0x3A, 0x49, 0x6E, 0x74,
    0x65, 0x72, 0x61, 0x63, 0x74, 0x69, 0x76, 0x65, 0x4F, 0x62, 0x6A, 0x65, 0x63, 0x74, 0x1B, 0x66,
    0x6C, 0x61, 0x73, 0x68, 0x2E, 0x64, 0x69, 0x73, 0x70, 0x6C, 0x61, 0x79, 0x3A, 0x44, 0x69, 0x73,
    0x70, 0x6C, 0x61, 0x79, 0x4F, 0x62, 0x6A, 0x65, 0x63, 0x74, 0x1C, 0x66, 0x6C, 0x61, 0x73, 0x68,
    0x2E, 0x65, 0x76, 0x65, 0x6E, 0x74, 0x73, 0x3A, 0x45, 0x76, 0x65, 0x6E, 0x74, 0x44, 0x69, 0x73,
    0x70, 0x61, 0x74, 0x63, 0x68, 0x65, 0x72, 0x04, 0x73, 0x74, 0x6F, 0x70, 0x0E, 0x61, 0x64, 0x64,
    0x46, 0x72, 0x61, 0x6D, 0x65, 0x53, 0x63, 0x72, 0x69, 0x70, 0x74, 0x06, 0x4F, 0x62, 0x6A, 0x65,
    0x63, 0x74, 0x0F, 0x45, 0x76, 0x65, 0x6E, 0x74, 0x44, 0x69, 0x73, 0x70, 0x61, 0x74, 0x63, 0x68,
    0x65, 0x72, 0x0D, 0x44, 0x69, 0x73, 0x70, 0x6C, 0x61, 0x79, 0x4F, 0x62, 0x6A, 0x65, 0x63, 0x74,
    0x11, 0x49, 0x6E, 0x74, 0x65, 0x72, 0x61, 0x63, 0x74, 0x69, 0x76, 0x65, 0x4F, 0x62, 0x6A, 0x65,
    0x63, 0x74, 0x16, 0x44, 0x69, 0x73, 0x70, 0x6C, 0x61, 0x79, 0x4F, 0x62, 0x6A, 0x65, 0x63, 0x74,
    0x43, 0x6F, 0x6E, 0x74, 0x61, 0x69, 0x6E, 0x65, 0x72, 0x06, 0x53, 0x70, 0x72, 0x69, 0x74, 0x65,
    0x27, 0x16, 0x01, 0x16, 0x03, 0x18, 0x05, 0x16, 0x06, 0x17, 0x01, 0x05, 0x00, 0x05, 0x00, 0x08,
    0x0E, 0x08, 0x0F, 0x16, 0x10, 0x08, 0x11, 0x16, 0x12, 0x16, 0x13, 0x08, 0x14, 0x16, 0x15, 0x16,
    0x16, 0x08, 0x17, 0x16, 0x18, 0x16, 0x19, 0x16, 0x1A, 0x08, 0x1B, 0x08, 0x1C, 0x08, 0x1D, 0x08,
    0x1E, 0x16, 0x1F, 0x16, 0x20, 0x16, 0x21, 0x08, 0x22, 0x16, 0x23, 0x16, 0x24, 0x08, 0x25, 0x1A,
    0x05, 0x1A, 0x26, 0x1A, 0x27, 0x1A, 0x28, 0x1A, 0x29, 0x1A, 0x2A, 0x1A, 0x2B, 0x02, 0x26, 0x06,
    0x07, 0x04, 0x01, 0x05, 0x08, 0x09, 0x0A, 0x0B, 0x02, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12,
    0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x03, 0x20, 0x21,
    0x22, 0x23, 0x24, 0x25, 0x26, 0x17, 0x07, 0x01, 0x02, 0x07, 0x02, 0x04, 0x07, 0x04, 0x07, 0x07,
    0x05, 0x08, 0x07, 0x05, 0x09, 0x07, 0x05, 0x0A, 0x07, 0x05, 0x0B, 0x07, 0x05, 0x0C, 0x09, 0x0D,
    0x01, 0x09, 0x2C, 0x01, 0x09, 0x2D, 0x01, 0x09, 0x08, 0x01, 0x09, 0x09, 0x01, 0x09, 0x0A, 0x01,
    0x09, 0x0B, 0x01, 0x09, 0x0C, 0x01, 0x07, 0x04, 0x2E, 0x07, 0x0D, 0x2F, 0x07, 0x02, 0x30, 0x07,
    0x02, 0x31, 0x07, 0x02, 0x32, 0x07, 0x02, 0x33, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x08, 0x03, 0x00,
    0x06, 0x06, 0x03, 0x00, 0x00, 0x02, 0x00, 0x04, 0x01, 0x00, 0x01, 0x05, 0x01, 0x00, 0x02, 0x06,
    0x01, 0x00, 0x03, 0x07, 0x01, 0x00, 0x04, 0x08, 0x01, 0x00, 0x05, 0x00, 0x00, 0x01, 0x07, 0x01,
    0x01, 0x04, 0x01, 0x00, 0x08, 0x00, 0x01, 0x01, 0x09, 0x0A, 0x03, 0xD0, 0x30, 0x47, 0x00, 0x00,
    0x01, 0x02, 0x01, 0x0A, 0x0B, 0x0C, 0xD0, 0x30, 0xD0, 0x27, 0x61, 0x09, 0x5D, 0x0A, 0x4F, 0x0A,
    0x00, 0x47, 0x00, 0x00, 0x02, 0x01, 0x01, 0x0A, 0x0B, 0x08, 0xD0, 0x30, 0x5D, 0x0A, 0x4F, 0x0A,
    0x00, 0x47, 0x00, 0x00, 0x03, 0x01, 0x01, 0x0A, 0x0B, 0x08, 0xD0, 0x30, 0x5D, 0x0A, 0x4F, 0x0A,
    0x00, 0x47, 0x00, 0x00, 0x04, 0x01, 0x01, 0x0A, 0x0B, 0x08, 0xD0, 0x30, 0x5D, 0x0A, 0x4F, 0x0A,
    0x00, 0x47, 0x00, 0x00, 0x05, 0x01, 0x01, 0x0A, 0x0B, 0x08, 0xD0, 0x30, 0x5D, 0x0A, 0x4F, 0x0A,
    0x00, 0x47, 0x00, 0x00, 0x06, 0x0B, 0x01, 0x0A, 0x0B, 0x24, 0xD0, 0x30, 0xD0, 0x49, 0x00, 0x5D,
    0x0B, 0x24, 0x00, 0xD0, 0x66, 0x0C, 0x24, 0x0B, 0xD0, 0x66, 0x0D, 0x24, 0x15, 0xD0, 0x66, 0x0E,
    0x24, 0x1F, 0xD0, 0x66, 0x0F, 0x24, 0x2A, 0xD0, 0x66, 0x10, 0x4F, 0x0B, 0x0A, 0x47, 0x00, 0x00,
    0x07, 0x02, 0x01, 0x01, 0x09, 0x27, 0xD0, 0x30, 0x65, 0x00, 0x60, 0x11, 0x30, 0x60, 0x12, 0x30,
    0x60, 0x13, 0x30, 0x60, 0x14, 0x30, 0x60, 0x15, 0x30, 0x60, 0x16, 0x30, 0x60, 0x02, 0x30, 0x60,
    0x02, 0x58, 0x00, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D, 0x68, 0x01, 0x47, 0x00, 0x00
};

// Multiplayer disabler decls
constexpr uint8_t DeclMultiplayerDisabled[] = {
    0x7B, 0x0D, 0x0A, 0x09, 0x69, 0x6E, 0x68, 0x65, 0x72, 0x69, 0x74, 0x20, 0x3D, 0x20, 0x22, 0x64,
    0x65, 0x66, 0x61, 0x75, 0x6C, 0x74, 0x5F, 0x74, 0x72, 0x61, 0x6E, 0x73, 0x69, 0x74, 0x69, 0x6F,
    0x6E, 0x22, 0x3B, 0x0D, 0x0A, 0x09, 0x65, 0x64, 0x69, 0x74, 0x20, 0x3D, 0x20, 0x7B, 0x0D, 0x0A,
    0x09, 0x09, 0x73, 0x77, 0x66, 0x49, 0x6E, 0x66, 0x6F, 0x20, 0x3D, 0x20, 0x7B, 0x0D, 0x0A, 0x09,
    0x09, 0x09, 0x73, 0x77, 0x66, 0x20, 0x3D, 0x20, 0x22, 0x73, 0x77, 0x66, 0x2F, 0x6D, 0x61, 0x69,
    0x6E, 0x5F, 0x6D, 0x65, 0x6E, 0x75, 0x2F, 0x73, 0x63, 0x72, 0x65, 0x65, 0x6E, 0x73, 0x2F, 0x67,
    0x65, 0x6E, 0x65, 0x72, 0x69, 0x63, 0x2E, 0x73, 0x77, 0x66, 0x22, 0x3B, 0x0D, 0x0A, 0x09, 0x09,
    0x7D, 0x0D, 0x0A, 0x09, 0x09, 0x63, 0x6C, 0x61, 0x73, 0x73, 0x20, 0x3D, 0x20, 0x22, 0x22, 0x3B,
    0x0D, 0x0A, 0x09, 0x09, 0x64, 0x69, 0x73, 0x70, 0x6C, 0x61, 0x79, 0x4E, 0x61, 0x6D, 0x65, 0x20,
    0x3D, 0x20, 0x22, 0x23, 0x73, 0x74, 0x72, 0x5F, 0x63, 0x6F, 0x64, 0x65, 0x5F, 0x6D, 0x61, 0x69,
    0x6E, 0x6D, 0x65, 0x6E, 0x75, 0x5F, 0x70, 0x6C, 0x61, 0x79, 0x5F, 0x6F, 0x6E, 0x6C, 0x69, 0x6E,
    0x65, 0x5F, 0x6E, 0x61, 0x6D, 0x65, 0x22, 0x3B, 0x0D, 0x0A, 0x09, 0x09, 0x67, 0x61, 0x6D, 0x65,
    0x49, 0x6E, 0x66, 0x6F, 0x20, 0x3D, 0x20, 0x7B, 0x0D, 0x0A, 0x09, 0x09, 0x09, 0x63, 0x61, 0x6D,
    0x65, 0x72, 0x61, 0x50, 0x6C, 0x61, 0x63, 0x65, 0x6D, 0x65, 0x6E, 0x74, 0x20, 0x3D, 0x20, 0x22,
    0x63, 0x61, 0x6D, 0x65, 0x72, 0x61, 0x5F, 0x70, 0x6C, 0x61, 0x79, 0x5F, 0x6F, 0x6E, 0x6C, 0x69,
    0x6E, 0x65, 0x22, 0x3B, 0x0D, 0x0A, 0x09, 0x09, 0x7D, 0x0D, 0x0A, 0x09, 0x09, 0x63, 0x68, 0x69,
    0x6C, 0x64, 0x45, 0x6C, 0x65, 0x6D, 0x65, 0x6E, 0x74, 0x73, 0x20, 0x3D, 0x20, 0x7B, 0x0D, 0x0A,
    0x09, 0x09, 0x09, 0x6E, 0x75, 0x6D, 0x20, 0x3D, 0x20, 0x31, 0x3B, 0x0D, 0x0A, 0x09, 0x09, 0x09,
    0x69, 0x74, 0x65, 0x6D, 0x5B, 0x30, 0x5D, 0x20, 0x3D, 0x20, 0x22, 0x6F, 0x70, 0x74, 0x69, 0x6F,
    0x6E, 0x73, 0x5F, 0x6C, 0x69, 0x73, 0x74, 0x5F, 0x63, 0x61, 0x74, 0x65, 0x67, 0x6F, 0x72, 0x79,
    0x22, 0x3B, 0x0D, 0x0A, 0x09, 0x09, 0x7D, 0x0D, 0x0A, 0x09, 0x7D, 0x0D, 0x0A, 0x7D
};

constexpr uint8_t DeclPlayOnlineDisabled[] = {
    0x7B, 0x0A, 0x09, 0x69, 0x6E, 0x68, 0x65, 0x72, 0x69, 0x74, 0x20, 0x3D, 0x20, 0x22, 0x64, 0x65,
    0x66, 0x61, 0x75, 0x6C, 0x74, 0x5F, 0x74, 0x72, 0x61, 0x6E, 0x73, 0x69, 0x74, 0x69, 0x6F, 0x6E,
    0x22, 0x3B, 0x0A, 0x09, 0x65, 0x64, 0x69, 0x74, 0x20, 0x3D, 0x20, 0x7B, 0x0A, 0x09, 0x09, 0x73,
    0x77, 0x66, 0x49, 0x6E, 0x66, 0x6F, 0x20, 0x3D, 0x20, 0x7B, 0x0A, 0x09, 0x09, 0x09, 0x73, 0x77,
    0x66, 0x20, 0x3D, 0x20, 0x22, 0x73, 0x77, 0x66, 0x2F, 0x68, 0x75, 0x64, 0x2F, 0x6D, 0x65, 0x6E,
    0x75, 0x73, 0x2F, 0x62, 0x61, 0x74, 0x74, 0x6C, 0x65, 0x5F, 0x61, 0x72, 0x65, 0x6E, 0x61, 0x2F,
    0x70, 0x6C, 0x61, 0x79, 0x5F, 0x6F, 0x6E, 0x6C, 0x69, 0x6E, 0x65, 0x5F, 0x73, 0x63, 0x72, 0x65,
    0x65, 0x6E, 0x2E, 0x73, 0x77, 0x66, 0x22, 0x3B, 0x0A, 0x09, 0x09, 0x7D, 0x0A, 0x09, 0x09, 0x63,
    0x6C, 0x61, 0x73, 0x73, 0x20, 0x3D, 0x20, 0x22, 0x22, 0x3B, 0x0A, 0x09, 0x09, 0x64, 0x69, 0x73,
    0x70, 0x6C, 0x61, 0x79, 0x4E, 0x61, 0x6D, 0x65, 0x20, 0x3D, 0x20, 0x22, 0x23, 0x73, 0x74, 0x72,
    0x5F, 0x63, 0x6F, 0x64, 0x65, 0x5F, 0x6D, 0x61, 0x69, 0x6E, 0x6D, 0x65, 0x6E, 0x75, 0x5F, 0x62,
    0x61, 0x74, 0x74, 0x6C, 0x65, 0x5F, 0x61, 0x72, 0x65, 0x6E, 0x61, 0x5F, 0x6E, 0x61, 0x6D, 0x65,
    0x22, 0x3B, 0x0A, 0x09, 0x09, 0x67, 0x61, 0x6D, 0x65, 0x49, 0x6E, 0x66, 0x6F, 0x20, 0x3D, 0x20,
    0x7B, 0x0A, 0x09, 0x09, 0x09, 0x63, 0x61, 0x6D, 0x65, 0x72, 0x61, 0x50, 0x6C, 0x61, 0x63, 0x65,
    0x6D, 0x65, 0x6E, 0x74, 0x20, 0x3D, 0x20, 0x22, 0x63, 0x61, 0x6D, 0x65, 0x72, 0x61, 0x5F, 0x70,
    0x6C, 0x61, 0x79, 0x5F, 0x6F, 0x6E, 0x6C, 0x69, 0x6E, 0x65, 0x22, 0x3B, 0x0A, 0x09, 0x09, 0x7D,
    0x0A, 0x09, 0x7D, 0x0A, 0x7D
};

// New strings for battlemode button
static const std::map<std::string, std::string> Languages = {
    { "french", "^8BATTLEMODE 2.0 (DÉSACTIVÉ)" },
    { "italian", "^8BATTLEMODE 2.0 (DISABILITATO)" },
    { "german", "^8BATTLEMODE 2.0 (DEAKTIVIERT)" },
    { "spanish", "^8BATTLEMODE 2.0 (DESHABILITADO)" },
    { "russian", "^8BATTLEMODE 2.0 (ОТКЛЮЧЕН)" },
    { "polish", "^8BATTLEMODE 2.0 (DEZAKTYWOWANY)" },
    { "japanese", "^8BATTLEMODE 2.0(無効)" },
    { "latin_spanish", "^8BATTLEMODE 2.0 (DESHABILITADO)" },
    { "portuguese", "^8BATTLEMODE 2.0 (DESABILITADO)" },
    { "traditional_chinese", "^8BATTLEMODE 2.0（已禁用）" },
    { "simplified_chinese", "^8BATTLEMODE 2.0（已禁用）" },
    { "korean", "^8BATTLEMODE 2.0(비활성화됨)" },
    { "english", "^8BATTLEMODE 2.0 (DISABLED)" }
};

// SWF files to replace
static const std::map<std::string, std::string> Swfs = {
    { "swf/hud/menus/battle_arena/play_online_screen.swf", "gameresources_patch2" },
    { "swf/hud/menus/battle_arena/lobby.swf", "gameresources_patch2" },
    { "swf/main_menu/screens/battle_arena.swf", "gameresources_patch1" },
    { "swf/main_menu/screens/match_browser.swf", "gameresources_patch1" }
};

// Horde Mode coin files to replace
static const std::map<std::string, std::string> HordeModeCoinDecls = {
    { "generated/decls/propitem/propitem/horde/point_coin.decl", "e6m1_cult_horde" },
    { "generated/decls/propitem/propitem/horde/point_coin.decl", "e6m2_earth_horde" },
    { "generated/decls/propitem/propitem/horde/point_coin.decl", "e6m3_mcity_horde" }
};

// Online safe keywords
static const std::vector<std::string> OnlineSafeModNameKeywords = {
    "/eternalmod/", ".tga", ".png", ".swf", ".bimage", "/advancedscreenviewshake/", "/audiolog/", "/audiologstory/", "/automap/", "/automapplayerprofile/",
    "/automapproperties/", "/automapsoundprofile/", "/env/", "/font/", "/fontfx/", "/fx/", "/gameitem/", "/globalfonttable/", "/gorebehavior/",
    "/gorecontainer/", "/gorewounds/", "/handsbobcycle/", "/highlightlos/", "/highlights/", "/hitconfirmationsoundsinfo/", "/hud/", "/hudelement/",
    "/lightrig/", "/lodgroup/", "/material2/", "/md6def/", "/modelasset/", "/particle/", "/particlestage/", "/renderlayerdefinition/", "/renderparm/",
    "/renderparmmeta/", "/renderprogflag/", "/ribbon2/", "/rumble/", "/soundevent/", "/soundpack/", "/soundrtpc/", "/soundstate/", "/soundswitch/",
    "/speaker/", "/staticimage/", "/swfresources/", "/uianchor/", "/uicolor/", "/weaponreticle/", "/weaponreticleswfinfo/", "/entitydef/light/", "/entitydef/fx",
    "/entitydef/", "/impacteffect/", "/uiweapon/", "/globalinitialwarehouse/", "/globalshell/", "/warehouseitem/", "/warehouseofflinecontainer/", "/tooltip/",
    "/livetile/", "/tutorialevent/", "/maps/game/dlc/", "/maps/game/dlc2/", "/maps/game/hub/", "/maps/game/shell/", "/maps/game/sp/", "/maps/game/tutorials/",
    "/decls/campaign"
};

// Online unsafe resource names
static const std::vector<std::string> UnsafeResourceNameKeywords = {
    "gameresources", "pvp", "shell", "warehouse", "e6"
};

/**
 * @brief Get the multiplayer disabler mods
 * 
 * @return Vector containing the multiplayer disabler mods
 */
std::vector<ResourceModFile> GetMultiplayerDisablerMods()
{
    // Get multiplayer disabler mods
    Mod parentMod;
    parentMod.LoadPriority = INT_MIN;

    std::vector<ResourceModFile> multiplayerDisablerMods;
    multiplayerDisablerMods.reserve(2 + Swfs.size() + Languages.size());

    // Battlemode
    ResourceModFile multiplayerDisablerDecl(parentMod, "generated/decls/menuelement/main_menu/screens/multiplayer.decl", "gameresources_patch2", false);
    multiplayerDisablerDecl.FileBytes = std::vector<std::byte>((std::byte*)DeclMultiplayerDisabled, (std::byte*)DeclMultiplayerDisabled + sizeof(DeclMultiplayerDisabled));
    multiplayerDisablerMods.push_back(multiplayerDisablerDecl);

    ResourceModFile playOnlineDisablerDecl(parentMod, "generated/decls/menuelement/main_menu/screens/battle_arena_play_online.decl", "gameresources_patch2", false);
    multiplayerDisablerDecl.FileBytes = std::vector<std::byte>((std::byte*)DeclPlayOnlineDisabled, (std::byte*)DeclPlayOnlineDisabled + sizeof(DeclPlayOnlineDisabled));
    multiplayerDisablerMods.push_back(playOnlineDisablerDecl);

    for (auto &swf : Swfs) {
        ResourceModFile multiplayerDisablerSwf(parentMod, swf.first, swf.second, false);
        multiplayerDisablerSwf.FileBytes = std::vector<std::byte>((std::byte*)GenericSwfData, (std::byte*)GenericSwfData + sizeof(GenericSwfData));
        multiplayerDisablerMods.push_back(multiplayerDisablerSwf);
    }

    for (auto &language : Languages) {
        ResourceModFile multiplayerDisablerBlang(parentMod, "EternalMod/strings/" + language.first + ".json", "gameresources_patch2", false);
        multiplayerDisablerBlang.IsBlangJson = true;
        std::string blangJson = R"({"strings":[{"name":"#str_code_mainmenu_play_online_name","text":"^8)" + language.second + R"("}]})";
        multiplayerDisablerBlang.FileBytes = std::vector<std::byte>((std::byte*)blangJson.c_str(), (std::byte*)blangJson.c_str() + blangJson.length());
        multiplayerDisablerMods.push_back(multiplayerDisablerBlang);
    }

    // Horde Mode
    ResourceModFile hordeModePointsDisablerDecl(parentMod, "generated/decls/scoringrubric/horde.decl", "gameresources_patch2", false);
    hordeModePointsDisablerDecl.FileBytes = std::vector<std::byte>((std::byte*)NullifiedScoringRubric, (std::byte*)NullifiedScoringRubric + sizeof(NullifiedScoringRubric));
    multiplayerDisablerMods.push_back(hordeModePointsDisablerDecl);

    ResourceModFile hordeModeChallengePointsDisablerDecl(parentMod, "generated/decls/scoringitem/horde/challenge_complete.decl", "gameresources_patch2", false);
    hordeModeChallengePointsDisablerDecl.FileBytes = std::vector<std::byte>((std::byte*)ChallengeCompleteScoreNullifier, (std::byte*)ChallengeCompleteScoreNullifier + sizeof(ChallengeCompleteScoreNullifier));
    multiplayerDisablerMods.push_back(hordeModeChallengePointsDisablerDecl);

    for (auto &decl : HordeModeCoinDecls) {
        ResourceModFile hordeModeCoinDisabler(parentMod, decl.first, decl.second, false);
        hordeModeCoinDisabler.FileBytes = std::vector<std::byte>((std::byte*)HordeCoinScoreNullifier, (std::byte*)HordeCoinScoreNullifier + sizeof(HordeCoinScoreNullifier));
        multiplayerDisablerMods.push_back(hordeModeCoinDisabler);
    }

    return multiplayerDisablerMods;
}

/**
 * @brief Checks wether a given mod's files is safe for online use
 * 
 * @param resourceModFiles Mod's resource mod files
 * @return True if the mod is safe for online, false otherwise 
 */
bool IsModSafeForOnline(const std::map<int32_t, std::vector<ResourceModFile>> &resourceModFiles)
{
    bool isSafe = true;
    bool isModifyingUnsafeResource = false;
    std::vector<ResourceModFile> assetsInfoJsons;

    for (const auto &resource : resourceModFiles) {
        for (const auto &modFile : resource.second) {
            if (EndsWith(ToLower(modFile.Name), "desktop.ini") || EndsWith(ToLower(modFile.Name), ".ds_store")) {
                continue;
            }

            // Check assets info files last
            if (modFile.IsAssetsInfoJson) {
                assetsInfoJsons.push_back(modFile);
                continue;
            }

            for (const auto &keyword : UnsafeResourceNameKeywords) {
                if (StartsWith(ToLower(modFile.ResourceName), keyword)) {
                    isModifyingUnsafeResource = true;
                    break;
                }
            }

            // Allow modification of anything outside of "generated/decls/"
            if (!StartsWith(ToLower(modFile.Name), "generated/decls/")) {
                continue;
            }

            if (isSafe) {
                bool found = false;

                for (const auto &keyword : OnlineSafeModNameKeywords) {
                    if (ToLower(modFile.Name).find(keyword) != std::string::npos) {
                        found = true;
                        break;
                    }
                }

                isSafe = found;
            }
        }
    }

    if (isSafe) {
        return true;
    }
    else if (isModifyingUnsafeResource) {
        return false;
    }

    // Don't allow adding unsafe mods in safe resource files into unsafe resources files
    // Otherwise, don't mark the mod as unsafe, it should be fine for single-player if
    // the mod is not modifying a critical resource
    for (const auto &assetsInfo : assetsInfoJsons) {
        if (assetsInfo.AssetsInfo.has_value()) {
            for (const auto &keyword : UnsafeResourceNameKeywords) {
                if (StartsWith(ToLower(assetsInfo.ResourceName), keyword)) {
                    return false;
                }
            }
        }
    }

    return true;
}
