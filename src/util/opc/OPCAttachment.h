#pragma once

#define OPC_ATTACHMENT_GROUND_TRUTH_FRAME 1

/**
 * An attachment to the point cloud.
 */
class OPCAttachment {
public:
    OPCAttachment(unsigned int pOpcType)
        : opc_type(pOpcType){}

    const unsigned int opc_type;
};
