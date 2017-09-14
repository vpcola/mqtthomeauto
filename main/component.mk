#
# Main Makefile. This is basically the same as a component makefile.
#

COMPONENT_ADD_INCLUDEDIRS := . MQTTClient-C/src MQTTClient-C/src/mbedtls MQTTPacket/src

COMPONENT_SRCDIRS := $(COMPONENT_ADD_INCLUDEDIRS)

