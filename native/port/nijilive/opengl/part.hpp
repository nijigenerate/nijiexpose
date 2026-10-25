#pragma once
#include "resources.hpp"
#include <core/render/part_draw_packet.hpp>
namespace nijiexpose::ogl {
using nicxlive::core::nodes::PartDrawPacket;
void oglInitPartBackendResources();
void oglDrawPartPacket(const PartDrawPacket& packet);
void oglExecutePartPacket(const PartDrawPacket& packet);
}
