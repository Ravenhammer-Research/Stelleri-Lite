/*
 * YangDataTypes.hpp
 * Generated enum of YANG module identifiers in PascalCase.
 */

#pragma once

namespace netcli {

enum class YangDataType {
  IanaBfdTypes,
  IanaCryptHash,
  IanaDotsSignalChannel,
  IanaHardware,
  IanaMsdTypes,
  IanaPseudowireTypes,
  IanaTlsProfile,
  IanaTunnelType,
  IetfAccessControlList,
  IetfAclTls,
  IetfAcldns,
  IetfAlarms,
  IetfBabel,
  IetfBfdIpMh,
  IetfBfdIpSh,
  IetfBfdLag,
  IetfBfdLarge,
  IetfBfdMpls,
  IetfBfdTypes,
  IetfBfdUnsolicited,
  IetfBfd,
  IetfComplexTypes,
  IetfConnectionOrientedOam,
  IetfConnectionlessOamMethods,
  IetfConnectionlessOam,
  IetfCryptoTypes,
  IetfDatastores,
  IetfDcFabricTopologyState,
  IetfDcFabricTopology,
  IetfDcFabricTypes,
  IetfDetnet,
  IetfDotsCallHome,
  IetfDotsDataChannel,
  IetfDotsMapping,
  IetfDotsRobustTrans,
  IetfDotsSignalChannel,
  IetfDotsTelemetry,
  IetfEthernetSegment,
  IetfEthertypes,
  IetfFactoryDefault,
  IetfFoo,
  IetfGeoLocation,
  IetfHardwareState,
  IetfHardware,
  IetfIgmpMldProxy,
  IetfIgmpMldSnooping,
  IetfIgmpMld,
  IetfInetTypes,
  IetfInterfaceProtection,
  IetfInterfaces,
  IetfIoam,
  IetfIp,
  IetfIpfixPsamp,
  IetfIpsecIptfs,
  IetfIsisReverseMetric,
  IetfIsis,
  IetfKeyChain,
  IetfKeystore,
  IetfLimeTimeTypes,
  IetfLmapCommon,
  IetfLmapControl,
  IetfLmapReport,
  IetfLogicalNetworkElement,
  IetfMicrowaveRadioLink,
  IetfMicrowaveTopology,
  IetfMicrowaveTypes,
  IetfModuleTagsState,
  IetfModuleTags,
  IetfMplsLdpExtended,
  IetfMplsLdp,
  IetfMplsMsd,
  IetfMpls,
  IetfMsdp,
  IetfMudDetextExample,
  IetfMudTls,
  IetfMudTransparency,
  IetfMud,
  IetfNetconfAcm,
  IetfNetconfMonitoring,
  IetfNetconfNmda,
  IetfNetconfNotifications,
  IetfNetconfPartialLock,
  IetfNetconfTime,
  IetfNetconfWithDefaults,
  IetfNetconf,
  IetfNetworkInstance,
  IetfNetworkState,
  IetfNetworkTopologyState,
  IetfNetworkTopology,
  IetfNetworkVpnPm,
  IetfNetwork,
  IetfNmdaCompare,
  IetfNotificationCapabilities,
  IetfNtp,
  IetfOrigin,
  IetfOspf,
  IetfPacketFields,
  IetfPimBase,
  IetfPimBidir,
  IetfPimDm,
  IetfPimRp,
  IetfPimSm,
  IetfPtp,
  IetfRestconfMonitoring,
  IetfRestconfSubscribedNotifications,
  IetfRestconf,
  IetfRibExtension,
  IetfRift,
  IetfRip,
  IetfRoutingPolicy,
  IetfRoutingTypes,
  IetfRouting,
  IetfSapNtw,
  IetfSchcCompoundAck,
  IetfSchc,
  IetfSegmentRoutingCommon,
  IetfSegmentRoutingMpls,
  IetfSegmentRouting,
  IetfServiceAssuranceDevice,
  IetfServiceAssuranceInterface,
  IetfServiceAssurance,
  IetfSidFile,
  IetfSnmpCommon,
  IetfSnmpCommunity,
  IetfSnmpEngine,
  IetfSnmpNotification,
  IetfSnmpProxy,
  IetfSnmpSsh,
  IetfSnmpTarget,
  IetfSnmpTls,
  IetfSnmpTsm,
  IetfSnmpUsm,
  IetfSnmpVacm,
  IetfSnmp,
  IetfSoftwireBr,
  IetfSoftwireCe,
  IetfSoftwireCommon,
  IetfSshClient,
  IetfSshCommon,
  IetfSshServer,
  IetfSubscribedNotifications,
  IetfSyslog,
  IetfSystemCapabilities,
  IetfSystemTacacsPlus,
  IetfSystem,
  IetfSztpBootstrapServer,
  IetfSztpConveyedInfo,
  IetfSztpCsr,
  IetfTcgAlgs,
  IetfTcpClient,
  IetfTcpCommon,
  IetfTcpServer,
  IetfTcp,
  IetfTePacketTypes,
  IetfTeTopologyState,
  IetfTeTopology,
  IetfTeTypes,
  IetfTemplate,
  IetfTlsClient,
  IetfTlsCommon,
  IetfTlsServer,
  IetfTpmRemoteAttestation,
  IetfTruststore,
  IetfTwamp,
  IetfVn,
  IetfVoucherRequest,
  IetfVoucher,
  IetfVpnCommon,
  IetfVrrp,
  IetfWsonTopology,
  IetfYangInstanceData,
  IetfYangLibrary,
  IetfYangMetadata,
  IetfYangPatch,
  IetfYangPush,
  IetfYangSchemaMount,
  IetfYangStructureExt,
  IetfYangTypes,
  IetfZtpTypes,
  Unknown
};

// Helpers: convert between YangDataType and canonical namespace URIs.
inline const char *toNamespace(YangDataType t) {
  using namespace std;
  switch (t) {
  case YangDataType::IanaBfdTypes:
    return "urn:ietf:params:xml:ns:yang:iana-bfd-types";
  case YangDataType::IanaCryptHash:
    return "urn:ietf:params:xml:ns:yang:iana-crypt-hash";
  case YangDataType::IanaDotsSignalChannel:
    return "urn:ietf:params:xml:ns:yang:iana-dots-signal-channel";
  case YangDataType::IanaHardware:
    return "urn:ietf:params:xml:ns:yang:iana-hardware";
  case YangDataType::IanaMsdTypes:
    return "urn:ietf:params:xml:ns:yang:iana-msd-types";
  case YangDataType::IanaPseudowireTypes:
    return "urn:ietf:params:xml:ns:yang:iana-pseudowire-types";
  case YangDataType::IanaTlsProfile:
    return "urn:ietf:params:xml:ns:yang:iana-tls-profile";
  case YangDataType::IanaTunnelType:
    return "urn:ietf:params:xml:ns:yang:iana-tunnel-type";
  case YangDataType::IetfAccessControlList:
    return "urn:ietf:params:xml:ns:yang:ietf-access-control-list";
  case YangDataType::IetfAclTls:
    return "urn:ietf:params:xml:ns:yang:ietf-acl-tls";
  case YangDataType::IetfAcldns:
    return "urn:ietf:params:xml:ns:yang:ietf-acldns";
  case YangDataType::IetfAlarms:
    return "urn:ietf:params:xml:ns:yang:ietf-alarms";
  case YangDataType::IetfBabel:
    return "urn:ietf:params:xml:ns:yang:ietf-babel";
  case YangDataType::IetfBfdIpMh:
    return "urn:ietf:params:xml:ns:yang:ietf-bfd-ip-mh";
  case YangDataType::IetfBfdIpSh:
    return "urn:ietf:params:xml:ns:yang:ietf-bfd-ip-sh";
  case YangDataType::IetfBfdLag:
    return "urn:ietf:params:xml:ns:yang:ietf-bfd-lag";
  case YangDataType::IetfBfdLarge:
    return "urn:ietf:params:xml:ns:yang:ietf-bfd-large";
  case YangDataType::IetfBfdMpls:
    return "urn:ietf:params:xml:ns:yang:ietf-bfd-mpls";
  case YangDataType::IetfBfdTypes:
    return "urn:ietf:params:xml:ns:yang:ietf-bfd-types";
  case YangDataType::IetfBfdUnsolicited:
    return "urn:ietf:params:xml:ns:yang:ietf-bfd-unsolicited";
  case YangDataType::IetfBfd:
    return "urn:ietf:params:xml:ns:yang:ietf-bfd";
  case YangDataType::IetfComplexTypes:
    return "urn:ietf:params:xml:ns:yang:ietf-complex-types";
  case YangDataType::IetfConnectionOrientedOam:
    return "urn:ietf:params:xml:ns:yang:ietf-connection-oriented-oam";
  case YangDataType::IetfConnectionlessOamMethods:
    return "urn:ietf:params:xml:ns:yang:ietf-connectionless-oam-methods";
  case YangDataType::IetfConnectionlessOam:
    return "urn:ietf:params:xml:ns:yang:ietf-connectionless-oam";
  case YangDataType::IetfCryptoTypes:
    return "urn:ietf:params:xml:ns:yang:ietf-crypto-types";
  case YangDataType::IetfDatastores:
    return "urn:ietf:params:xml:ns:yang:ietf-datastores";
  case YangDataType::IetfDcFabricTopologyState:
    return "urn:ietf:params:xml:ns:yang:ietf-dc-fabric-topology-state";
  case YangDataType::IetfDcFabricTopology:
    return "urn:ietf:params:xml:ns:yang:ietf-dc-fabric-topology";
  case YangDataType::IetfDcFabricTypes:
    return "urn:ietf:params:xml:ns:yang:ietf-dc-fabric-types";
  case YangDataType::IetfDetnet:
    return "urn:ietf:params:xml:ns:yang:ietf-detnet";
  case YangDataType::IetfDotsCallHome:
    return "urn:ietf:params:xml:ns:yang:ietf-dots-call-home";
  case YangDataType::IetfDotsDataChannel:
    return "urn:ietf:params:xml:ns:yang:ietf-dots-data-channel";
  case YangDataType::IetfDotsMapping:
    return "urn:ietf:params:xml:ns:yang:ietf-dots-mapping";
  case YangDataType::IetfDotsRobustTrans:
    return "urn:ietf:params:xml:ns:yang:ietf-dots-robust-trans";
  case YangDataType::IetfDotsSignalChannel:
    return "urn:ietf:params:xml:ns:yang:ietf-dots-signal-channel";
  case YangDataType::IetfDotsTelemetry:
    return "urn:ietf:params:xml:ns:yang:ietf-dots-telemetry";
  case YangDataType::IetfEthernetSegment:
    return "urn:ietf:params:xml:ns:yang:ietf-ethernet-segment";
  case YangDataType::IetfEthertypes:
    return "urn:ietf:params:xml:ns:yang:ietf-ethertypes";
  case YangDataType::IetfFactoryDefault:
    return "urn:ietf:params:xml:ns:yang:ietf-factory-default";
  case YangDataType::IetfFoo:
    return "urn:ietf:params:xml:ns:yang:ietf-foo";
  case YangDataType::IetfGeoLocation:
    return "urn:ietf:params:xml:ns:yang:ietf-geo-location";
  case YangDataType::IetfHardwareState:
    return "urn:ietf:params:xml:ns:yang:ietf-hardware-state";
  case YangDataType::IetfHardware:
    return "urn:ietf:params:xml:ns:yang:ietf-hardware";
  case YangDataType::IetfIgmpMldProxy:
    return "urn:ietf:params:xml:ns:yang:ietf-igmp-mld-proxy";
  case YangDataType::IetfIgmpMldSnooping:
    return "urn:ietf:params:xml:ns:yang:ietf-igmp-mld-snooping";
  case YangDataType::IetfIgmpMld:
    return "urn:ietf:params:xml:ns:yang:ietf-igmp-mld";
  case YangDataType::IetfInetTypes:
    return "urn:ietf:params:xml:ns:yang:ietf-inet-types";
  case YangDataType::IetfInterfaceProtection:
    return "urn:ietf:params:xml:ns:yang:ietf-interface-protection";
  case YangDataType::IetfInterfaces:
    return "urn:ietf:params:xml:ns:yang:ietf-interfaces";
  case YangDataType::IetfIoam:
    return "urn:ietf:params:xml:ns:yang:ietf-ioam";
  case YangDataType::IetfIp:
    return "urn:ietf:params:xml:ns:yang:ietf-ip";
  case YangDataType::IetfIpfixPsamp:
    return "urn:ietf:params:xml:ns:yang:ietf-ipfix-psamp";
  case YangDataType::IetfIpsecIptfs:
    return "urn:ietf:params:xml:ns:yang:ietf-ipsec-iptfs";
  case YangDataType::IetfIsisReverseMetric:
    return "urn:ietf:params:xml:ns:yang:ietf-isis-reverse-metric";
  case YangDataType::IetfIsis:
    return "urn:ietf:params:xml:ns:yang:ietf-isis";
  case YangDataType::IetfKeyChain:
    return "urn:ietf:params:xml:ns:yang:ietf-key-chain";
  case YangDataType::IetfKeystore:
    return "urn:ietf:params:xml:ns:yang:ietf-keystore";
  case YangDataType::IetfLimeTimeTypes:
    return "urn:ietf:params:xml:ns:yang:ietf-lime-time-types";
  case YangDataType::IetfLmapCommon:
    return "urn:ietf:params:xml:ns:yang:ietf-lmap-common";
  case YangDataType::IetfLmapControl:
    return "urn:ietf:params:xml:ns:yang:ietf-lmap-control";
  case YangDataType::IetfLmapReport:
    return "urn:ietf:params:xml:ns:yang:ietf-lmap-report";
  case YangDataType::IetfLogicalNetworkElement:
    return "urn:ietf:params:xml:ns:yang:ietf-logical-network-element";
  case YangDataType::IetfMicrowaveRadioLink:
    return "urn:ietf:params:xml:ns:yang:ietf-microwave-radio-link";
  case YangDataType::IetfMicrowaveTopology:
    return "urn:ietf:params:xml:ns:yang:ietf-microwave-topology";
  case YangDataType::IetfMicrowaveTypes:
    return "urn:ietf:params:xml:ns:yang:ietf-microwave-types";
  case YangDataType::IetfModuleTagsState:
    return "urn:ietf:params:xml:ns:yang:ietf-module-tags-state";
  case YangDataType::IetfModuleTags:
    return "urn:ietf:params:xml:ns:yang:ietf-module-tags";
  case YangDataType::IetfMplsLdpExtended:
    return "urn:ietf:params:xml:ns:yang:ietf-mpls-ldp-extended";
  case YangDataType::IetfMplsLdp:
    return "urn:ietf:params:xml:ns:yang:ietf-mpls-ldp";
  case YangDataType::IetfMplsMsd:
    return "urn:ietf:params:xml:ns:yang:ietf-mpls-msd";
  case YangDataType::IetfMpls:
    return "urn:ietf:params:xml:ns:yang:ietf-mpls";
  case YangDataType::IetfMsdp:
    return "urn:ietf:params:xml:ns:yang:ietf-msdp";
  case YangDataType::IetfMudDetextExample:
    return "urn:ietf:params:xml:ns:yang:ietf-mud-detext-example";
  case YangDataType::IetfMudTls:
    return "urn:ietf:params:xml:ns:yang:ietf-mud-tls";
  case YangDataType::IetfMudTransparency:
    return "urn:ietf:params:xml:ns:yang:ietf-mud-transparency";
  case YangDataType::IetfMud:
    return "urn:ietf:params:xml:ns:yang:ietf-mud";
  case YangDataType::IetfNetconfAcm:
    return "urn:ietf:params:xml:ns:yang:ietf-netconf-acm";
  case YangDataType::IetfNetconfMonitoring:
    return "urn:ietf:params:xml:ns:yang:ietf-netconf-monitoring";
  case YangDataType::IetfNetconfNmda:
    return "urn:ietf:params:xml:ns:yang:ietf-netconf-nmda";
  case YangDataType::IetfNetconfNotifications:
    return "urn:ietf:params:xml:ns:yang:ietf-netconf-notifications";
  case YangDataType::IetfNetconfPartialLock:
    return "urn:ietf:params:xml:ns:yang:ietf-netconf-partial-lock";
  case YangDataType::IetfNetconfTime:
    return "urn:ietf:params:xml:ns:yang:ietf-netconf-time";
  case YangDataType::IetfNetconfWithDefaults:
    return "urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults";
  case YangDataType::IetfNetconf:
    return "urn:ietf:params:xml:ns:yang:ietf-netconf";
  case YangDataType::IetfNetworkInstance:
    return "urn:ietf:params:xml:ns:yang:ietf-network-instance";
  case YangDataType::IetfNetworkState:
    return "urn:ietf:params:xml:ns:yang:ietf-network-state";
  case YangDataType::IetfNetworkTopologyState:
    return "urn:ietf:params:xml:ns:yang:ietf-network-topology-state";
  case YangDataType::IetfNetworkTopology:
    return "urn:ietf:params:xml:ns:yang:ietf-network-topology";
  case YangDataType::IetfNetworkVpnPm:
    return "urn:ietf:params:xml:ns:yang:ietf-network-vpn-pm";
  case YangDataType::IetfNetwork:
    return "urn:ietf:params:xml:ns:yang:ietf-network";
  case YangDataType::IetfNmdaCompare:
    return "urn:ietf:params:xml:ns:yang:ietf-nmda-compare";
  case YangDataType::IetfNotificationCapabilities:
    return "urn:ietf:params:xml:ns:yang:ietf-notification-capabilities";
  case YangDataType::IetfNtp:
    return "urn:ietf:params:xml:ns:yang:ietf-ntp";
  case YangDataType::IetfOrigin:
    return "urn:ietf:params:xml:ns:yang:ietf-origin";
  case YangDataType::IetfOspf:
    return "urn:ietf:params:xml:ns:yang:ietf-ospf";
  case YangDataType::IetfPacketFields:
    return "urn:ietf:params:xml:ns:yang:ietf-packet-fields";
  case YangDataType::IetfPimBase:
    return "urn:ietf:params:xml:ns:yang:ietf-pim-base";
  case YangDataType::IetfPimBidir:
    return "urn:ietf:params:xml:ns:yang:ietf-pim-bidir";
  case YangDataType::IetfPimDm:
    return "urn:ietf:params:xml:ns:yang:ietf-pim-dm";
  case YangDataType::IetfPimRp:
    return "urn:ietf:params:xml:ns:yang:ietf-pim-rp";
  case YangDataType::IetfPimSm:
    return "urn:ietf:params:xml:ns:yang:ietf-pim-sm";
  case YangDataType::IetfPtp:
    return "urn:ietf:params:xml:ns:yang:ietf-ptp";
  case YangDataType::IetfRestconfMonitoring:
    return "urn:ietf:params:xml:ns:yang:ietf-restconf-monitoring";
  case YangDataType::IetfRestconfSubscribedNotifications:
    return "urn:ietf:params:xml:ns:yang:ietf-restconf-subscribed-notifications";
  case YangDataType::IetfRestconf:
    return "urn:ietf:params:xml:ns:yang:ietf-restconf";
  case YangDataType::IetfRibExtension:
    return "urn:ietf:params:xml:ns:yang:ietf-rib-extension";
  case YangDataType::IetfRift:
    return "urn:ietf:params:xml:ns:yang:ietf-rift";
  case YangDataType::IetfRip:
    return "urn:ietf:params:xml:ns:yang:ietf-rip";
  case YangDataType::IetfRoutingPolicy:
    return "urn:ietf:params:xml:ns:yang:ietf-routing-policy";
  case YangDataType::IetfRoutingTypes:
    return "urn:ietf:params:xml:ns:yang:ietf-routing-types";
  case YangDataType::IetfRouting:
    return "urn:ietf:params:xml:ns:yang:ietf-routing";
  case YangDataType::IetfSapNtw:
    return "urn:ietf:params:xml:ns:yang:ietf-sap-ntw";
  case YangDataType::IetfSchcCompoundAck:
    return "urn:ietf:params:xml:ns:yang:ietf-schc-compound-ack";
  case YangDataType::IetfSchc:
    return "urn:ietf:params:xml:ns:yang:ietf-schc";
  case YangDataType::IetfSegmentRoutingCommon:
    return "urn:ietf:params:xml:ns:yang:ietf-segment-routing-common";
  case YangDataType::IetfSegmentRoutingMpls:
    return "urn:ietf:params:xml:ns:yang:ietf-segment-routing-mpls";
  case YangDataType::IetfSegmentRouting:
    return "urn:ietf:params:xml:ns:yang:ietf-segment-routing";
  case YangDataType::IetfServiceAssuranceDevice:
    return "urn:ietf:params:xml:ns:yang:ietf-service-assurance-device";
  case YangDataType::IetfServiceAssuranceInterface:
    return "urn:ietf:params:xml:ns:yang:ietf-service-assurance-interface";
  case YangDataType::IetfServiceAssurance:
    return "urn:ietf:params:xml:ns:yang:ietf-service-assurance";
  case YangDataType::IetfSidFile:
    return "urn:ietf:params:xml:ns:yang:ietf-sid-file";
  case YangDataType::IetfSnmpCommon:
    return "urn:ietf:params:xml:ns:yang:ietf-snmp-common";
  case YangDataType::IetfSnmpCommunity:
    return "urn:ietf:params:xml:ns:yang:ietf-snmp-community";
  case YangDataType::IetfSnmpEngine:
    return "urn:ietf:params:xml:ns:yang:ietf-snmp-engine";
  case YangDataType::IetfSnmpNotification:
    return "urn:ietf:params:xml:ns:yang:ietf-snmp-notification";
  case YangDataType::IetfSnmpProxy:
    return "urn:ietf:params:xml:ns:yang:ietf-snmp-proxy";
  case YangDataType::IetfSnmpSsh:
    return "urn:ietf:params:xml:ns:yang:ietf-snmp-ssh";
  case YangDataType::IetfSnmpTarget:
    return "urn:ietf:params:xml:ns:yang:ietf-snmp-target";
  case YangDataType::IetfSnmpTls:
    return "urn:ietf:params:xml:ns:yang:ietf-snmp-tls";
  case YangDataType::IetfSnmpTsm:
    return "urn:ietf:params:xml:ns:yang:ietf-snmp-tsm";
  case YangDataType::IetfSnmpUsm:
    return "urn:ietf:params:xml:ns:yang:ietf-snmp-usm";
  case YangDataType::IetfSnmpVacm:
    return "urn:ietf:params:xml:ns:yang:ietf-snmp-vacm";
  case YangDataType::IetfSnmp:
    return "urn:ietf:params:xml:ns:yang:ietf-snmp";
  case YangDataType::IetfSoftwireBr:
    return "urn:ietf:params:xml:ns:yang:ietf-softwire-br";
  case YangDataType::IetfSoftwireCe:
    return "urn:ietf:params:xml:ns:yang:ietf-softwire-ce";
  case YangDataType::IetfSoftwireCommon:
    return "urn:ietf:params:xml:ns:yang:ietf-softwire-common";
  case YangDataType::IetfSshClient:
    return "urn:ietf:params:xml:ns:yang:ietf-ssh-client";
  case YangDataType::IetfSshCommon:
    return "urn:ietf:params:xml:ns:yang:ietf-ssh-common";
  case YangDataType::IetfSshServer:
    return "urn:ietf:params:xml:ns:yang:ietf-ssh-server";
  case YangDataType::IetfSubscribedNotifications:
    return "urn:ietf:params:xml:ns:yang:ietf-subscribed-notifications";
  case YangDataType::IetfSyslog:
    return "urn:ietf:params:xml:ns:yang:ietf-syslog";
  case YangDataType::IetfSystemCapabilities:
    return "urn:ietf:params:xml:ns:yang:ietf-system-capabilities";
  case YangDataType::IetfSystemTacacsPlus:
    return "urn:ietf:params:xml:ns:yang:ietf-system-tacacs-plus";
  case YangDataType::IetfSystem:
    return "urn:ietf:params:xml:ns:yang:ietf-system";
  case YangDataType::IetfSztpBootstrapServer:
    return "urn:ietf:params:xml:ns:yang:ietf-sztp-bootstrap-server";
  case YangDataType::IetfSztpConveyedInfo:
    return "urn:ietf:params:xml:ns:yang:ietf-sztp-conveyed-info";
  case YangDataType::IetfSztpCsr:
    return "urn:ietf:params:xml:ns:yang:ietf-sztp-csr";
  case YangDataType::IetfTcgAlgs:
    return "urn:ietf:params:xml:ns:yang:ietf-tcg-algs";
  case YangDataType::IetfTcpClient:
    return "urn:ietf:params:xml:ns:yang:ietf-tcp-client";
  case YangDataType::IetfTcpCommon:
    return "urn:ietf:params:xml:ns:yang:ietf-tcp-common";
  case YangDataType::IetfTcpServer:
    return "urn:ietf:params:xml:ns:yang:ietf-tcp-server";
  case YangDataType::IetfTcp:
    return "urn:ietf:params:xml:ns:yang:ietf-tcp";
  case YangDataType::IetfTePacketTypes:
    return "urn:ietf:params:xml:ns:yang:ietf-te-packet-types";
  case YangDataType::IetfTeTopologyState:
    return "urn:ietf:params:xml:ns:yang:ietf-te-topology-state";
  case YangDataType::IetfTeTopology:
    return "urn:ietf:params:xml:ns:yang:ietf-te-topology";
  case YangDataType::IetfTeTypes:
    return "urn:ietf:params:xml:ns:yang:ietf-te-types";
  case YangDataType::IetfTemplate:
    return "urn:ietf:params:xml:ns:yang:ietf-template";
  case YangDataType::IetfTlsClient:
    return "urn:ietf:params:xml:ns:yang:ietf-tls-client";
  case YangDataType::IetfTlsCommon:
    return "urn:ietf:params:xml:ns:yang:ietf-tls-common";
  case YangDataType::IetfTlsServer:
    return "urn:ietf:params:xml:ns:yang:ietf-tls-server";
  case YangDataType::IetfTpmRemoteAttestation:
    return "urn:ietf:params:xml:ns:yang:ietf-tpm-remote-attestation";
  case YangDataType::IetfTruststore:
    return "urn:ietf:params:xml:ns:yang:ietf-truststore";
  case YangDataType::IetfTwamp:
    return "urn:ietf:params:xml:ns:yang:ietf-twamp";
  case YangDataType::IetfVn:
    return "urn:ietf:params:xml:ns:yang:ietf-vn";
  case YangDataType::IetfVoucherRequest:
    return "urn:ietf:params:xml:ns:yang:ietf-voucher-request";
  case YangDataType::IetfVoucher:
    return "urn:ietf:params:xml:ns:yang:ietf-voucher";
  case YangDataType::IetfVpnCommon:
    return "urn:ietf:params:xml:ns:yang:ietf-vpn-common";
  case YangDataType::IetfVrrp:
    return "urn:ietf:params:xml:ns:yang:ietf-vrrp";
  case YangDataType::IetfWsonTopology:
    return "urn:ietf:params:xml:ns:yang:ietf-wson-topology";
  case YangDataType::IetfYangInstanceData:
    return "urn:ietf:params:xml:ns:yang:ietf-yang-instance-data";
  case YangDataType::IetfYangLibrary:
    return "urn:ietf:params:xml:ns:yang:ietf-yang-library";
  case YangDataType::IetfYangMetadata:
    return "urn:ietf:params:xml:ns:yang:ietf-yang-metadata";
  case YangDataType::IetfYangPatch:
    return "urn:ietf:params:xml:ns:yang:ietf-yang-patch";
  case YangDataType::IetfYangPush:
    return "urn:ietf:params:xml:ns:yang:ietf-yang-push";
  case YangDataType::IetfYangSchemaMount:
    return "urn:ietf:params:xml:ns:yang:ietf-yang-schema-mount";
  case YangDataType::IetfYangStructureExt:
    return "urn:ietf:params:xml:ns:yang:ietf-yang-structure-ext";
  case YangDataType::IetfYangTypes:
    return "urn:ietf:params:xml:ns:yang:ietf-yang-types";
  case YangDataType::IetfZtpTypes:
    return "urn:ietf:params:xml:ns:yang:ietf-ztp-types";
  case YangDataType::Unknown:
  default:
    return "";
  }
}

inline YangDataType fromNamespace(const std::string &ns) {
  using namespace std;
  if (ns.empty())
    return YangDataType::Unknown;

  // Match canonical URN form first.
  static const unordered_map<string, YangDataType> m = {
    {"urn:ietf:params:xml:ns:yang:iana-bfd-types", YangDataType::IanaBfdTypes},
    {"urn:ietf:params:xml:ns:yang:iana-crypt-hash", YangDataType::IanaCryptHash},
    {"urn:ietf:params:xml:ns:yang:iana-dots-signal-channel", YangDataType::IanaDotsSignalChannel},
    {"urn:ietf:params:xml:ns:yang:iana-hardware", YangDataType::IanaHardware},
    {"urn:ietf:params:xml:ns:yang:iana-msd-types", YangDataType::IanaMsdTypes},
    {"urn:ietf:params:xml:ns:yang:iana-pseudowire-types", YangDataType::IanaPseudowireTypes},
    {"urn:ietf:params:xml:ns:yang:iana-tls-profile", YangDataType::IanaTlsProfile},
    {"urn:ietf:params:xml:ns:yang:iana-tunnel-type", YangDataType::IanaTunnelType},
    {"urn:ietf:params:xml:ns:yang:ietf-access-control-list", YangDataType::IetfAccessControlList},
    {"urn:ietf:params:xml:ns:yang:ietf-acl-tls", YangDataType::IetfAclTls},
    {"urn:ietf:params:xml:ns:yang:ietf-acldns", YangDataType::IetfAcldns},
    {"urn:ietf:params:xml:ns:yang:ietf-alarms", YangDataType::IetfAlarms},
    {"urn:ietf:params:xml:ns:yang:ietf-babel", YangDataType::IetfBabel},
    {"urn:ietf:params:xml:ns:yang:ietf-bfd-ip-mh", YangDataType::IetfBfdIpMh},
    {"urn:ietf:params:xml:ns:yang:ietf-bfd-ip-sh", YangDataType::IetfBfdIpSh},
    {"urn:ietf:params:xml:ns:yang:ietf-bfd-lag", YangDataType::IetfBfdLag},
    {"urn:ietf:params:xml:ns:yang:ietf-bfd-large", YangDataType::IetfBfdLarge},
    {"urn:ietf:params:xml:ns:yang:ietf-bfd-mpls", YangDataType::IetfBfdMpls},
    {"urn:ietf:params:xml:ns:yang:ietf-bfd-types", YangDataType::IetfBfdTypes},
    {"urn:ietf:params:xml:ns:yang:ietf-bfd-unsolicited", YangDataType::IetfBfdUnsolicited},
    {"urn:ietf:params:xml:ns:yang:ietf-bfd", YangDataType::IetfBfd},
    {"urn:ietf:params:xml:ns:yang:ietf-complex-types", YangDataType::IetfComplexTypes},
    {"urn:ietf:params:xml:ns:yang:ietf-connection-oriented-oam", YangDataType::IetfConnectionOrientedOam},
    {"urn:ietf:params:xml:ns:yang:ietf-connectionless-oam-methods", YangDataType::IetfConnectionlessOamMethods},
    {"urn:ietf:params:xml:ns:yang:ietf-connectionless-oam", YangDataType::IetfConnectionlessOam},
    {"urn:ietf:params:xml:ns:yang:ietf-crypto-types", YangDataType::IetfCryptoTypes},
    {"urn:ietf:params:xml:ns:yang:ietf-datastores", YangDataType::IetfDatastores},
    {"urn:ietf:params:xml:ns:yang:ietf-dc-fabric-topology-state", YangDataType::IetfDcFabricTopologyState},
    {"urn:ietf:params:xml:ns:yang:ietf-dc-fabric-topology", YangDataType::IetfDcFabricTopology},
    {"urn:ietf:params:xml:ns:yang:ietf-dc-fabric-types", YangDataType::IetfDcFabricTypes},
    {"urn:ietf:params:xml:ns:yang:ietf-detnet", YangDataType::IetfDetnet},
    {"urn:ietf:params:xml:ns:yang:ietf-dots-call-home", YangDataType::IetfDotsCallHome},
    {"urn:ietf:params:xml:ns:yang:ietf-dots-data-channel", YangDataType::IetfDotsDataChannel},
    {"urn:ietf:params:xml:ns:yang:ietf-dots-mapping", YangDataType::IetfDotsMapping},
    {"urn:ietf:params:xml:ns:yang:ietf-dots-robust-trans", YangDataType::IetfDotsRobustTrans},
    {"urn:ietf:params:xml:ns:yang:ietf-dots-signal-channel", YangDataType::IetfDotsSignalChannel},
    {"urn:ietf:params:xml:ns:yang:ietf-dots-telemetry", YangDataType::IetfDotsTelemetry},
    {"urn:ietf:params:xml:ns:yang:ietf-ethernet-segment", YangDataType::IetfEthernetSegment},
    {"urn:ietf:params:xml:ns:yang:ietf-ethertypes", YangDataType::IetfEthertypes},
    {"urn:ietf:params:xml:ns:yang:ietf-factory-default", YangDataType::IetfFactoryDefault},
    {"urn:ietf:params:xml:ns:yang:ietf-foo", YangDataType::IetfFoo},
    {"urn:ietf:params:xml:ns:yang:ietf-geo-location", YangDataType::IetfGeoLocation},
    {"urn:ietf:params:xml:ns:yang:ietf-hardware-state", YangDataType::IetfHardwareState},
    {"urn:ietf:params:xml:ns:yang:ietf-hardware", YangDataType::IetfHardware},
    {"urn:ietf:params:xml:ns:yang:ietf-igmp-mld-proxy", YangDataType::IetfIgmpMldProxy},
    {"urn:ietf:params:xml:ns:yang:ietf-igmp-mld-snooping", YangDataType::IetfIgmpMldSnooping},
    {"urn:ietf:params:xml:ns:yang:ietf-igmp-mld", YangDataType::IetfIgmpMld},
    {"urn:ietf:params:xml:ns:yang:ietf-inet-types", YangDataType::IetfInetTypes},
    {"urn:ietf:params:xml:ns:yang:ietf-interface-protection", YangDataType::IetfInterfaceProtection},
    {"urn:ietf:params:xml:ns:yang:ietf-interfaces", YangDataType::IetfInterfaces},
    {"urn:ietf:params:xml:ns:yang:ietf-ioam", YangDataType::IetfIoam},
    {"urn:ietf:params:xml:ns:yang:ietf-ip", YangDataType::IetfIp},
    {"urn:ietf:params:xml:ns:yang:ietf-ipfix-psamp", YangDataType::IetfIpfixPsamp},
    {"urn:ietf:params:xml:ns:yang:ietf-ipsec-iptfs", YangDataType::IetfIpsecIptfs},
    {"urn:ietf:params:xml:ns:yang:ietf-isis-reverse-metric", YangDataType::IetfIsisReverseMetric},
    {"urn:ietf:params:xml:ns:yang:ietf-isis", YangDataType::IetfIsis},
    {"urn:ietf:params:xml:ns:yang:ietf-key-chain", YangDataType::IetfKeyChain},
    {"urn:ietf:params:xml:ns:yang:ietf-keystore", YangDataType::IetfKeystore},
    {"urn:ietf:params:xml:ns:yang:ietf-lime-time-types", YangDataType::IetfLimeTimeTypes},
    {"urn:ietf:params:xml:ns:yang:ietf-lmap-common", YangDataType::IetfLmapCommon},
    {"urn:ietf:params:xml:ns:yang:ietf-lmap-control", YangDataType::IetfLmapControl},
    {"urn:ietf:params:xml:ns:yang:ietf-lmap-report", YangDataType::IetfLmapReport},
    {"urn:ietf:params:xml:ns:yang:ietf-logical-network-element", YangDataType::IetfLogicalNetworkElement},
    {"urn:ietf:params:xml:ns:yang:ietf-microwave-radio-link", YangDataType::IetfMicrowaveRadioLink},
    {"urn:ietf:params:xml:ns:yang:ietf-microwave-topology", YangDataType::IetfMicrowaveTopology},
    {"urn:ietf:params:xml:ns:yang:ietf-microwave-types", YangDataType::IetfMicrowaveTypes},
    {"urn:ietf:params:xml:ns:yang:ietf-module-tags-state", YangDataType::IetfModuleTagsState},
    {"urn:ietf:params:xml:ns:yang:ietf-module-tags", YangDataType::IetfModuleTags},
    {"urn:ietf:params:xml:ns:yang:ietf-mpls-ldp-extended", YangDataType::IetfMplsLdpExtended},
    {"urn:ietf:params:xml:ns:yang:ietf-mpls-ldp", YangDataType::IetfMplsLdp},
    {"urn:ietf:params:xml:ns:yang:ietf-mpls-msd", YangDataType::IetfMplsMsd},
    {"urn:ietf:params:xml:ns:yang:ietf-mpls", YangDataType::IetfMpls},
    {"urn:ietf:params:xml:ns:yang:ietf-msdp", YangDataType::IetfMsdp},
    {"urn:ietf:params:xml:ns:yang:ietf-mud-detext-example", YangDataType::IetfMudDetextExample},
    {"urn:ietf:params:xml:ns:yang:ietf-mud-tls", YangDataType::IetfMudTls},
    {"urn:ietf:params:xml:ns:yang:ietf-mud-transparency", YangDataType::IetfMudTransparency},
    {"urn:ietf:params:xml:ns:yang:ietf-mud", YangDataType::IetfMud},
    {"urn:ietf:params:xml:ns:yang:ietf-netconf-acm", YangDataType::IetfNetconfAcm},
    {"urn:ietf:params:xml:ns:yang:ietf-netconf-monitoring", YangDataType::IetfNetconfMonitoring},
    {"urn:ietf:params:xml:ns:yang:ietf-netconf-nmda", YangDataType::IetfNetconfNmda},
    {"urn:ietf:params:xml:ns:yang:ietf-netconf-notifications", YangDataType::IetfNetconfNotifications},
    {"urn:ietf:params:xml:ns:yang:ietf-netconf-partial-lock", YangDataType::IetfNetconfPartialLock},
    {"urn:ietf:params:xml:ns:yang:ietf-netconf-time", YangDataType::IetfNetconfTime},
    {"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults", YangDataType::IetfNetconfWithDefaults},
    {"urn:ietf:params:xml:ns:yang:ietf-netconf", YangDataType::IetfNetconf},
    {"urn:ietf:params:xml:ns:yang:ietf-network-instance", YangDataType::IetfNetworkInstance},
    {"urn:ietf:params:xml:ns:yang:ietf-network-state", YangDataType::IetfNetworkState},
    {"urn:ietf:params:xml:ns:yang:ietf-network-topology-state", YangDataType::IetfNetworkTopologyState},
    {"urn:ietf:params:xml:ns:yang:ietf-network-topology", YangDataType::IetfNetworkTopology},
    {"urn:ietf:params:xml:ns:yang:ietf-network-vpn-pm", YangDataType::IetfNetworkVpnPm},
    {"urn:ietf:params:xml:ns:yang:ietf-network", YangDataType::IetfNetwork},
    {"urn:ietf:params:xml:ns:yang:ietf-nmda-compare", YangDataType::IetfNmdaCompare},
    {"urn:ietf:params:xml:ns:yang:ietf-notification-capabilities", YangDataType::IetfNotificationCapabilities},
    {"urn:ietf:params:xml:ns:yang:ietf-ntp", YangDataType::IetfNtp},
    {"urn:ietf:params:xml:ns:yang:ietf-origin", YangDataType::IetfOrigin},
    {"urn:ietf:params:xml:ns:yang:ietf-ospf", YangDataType::IetfOspf},
    {"urn:ietf:params:xml:ns:yang:ietf-packet-fields", YangDataType::IetfPacketFields},
    {"urn:ietf:params:xml:ns:yang:ietf-pim-base", YangDataType::IetfPimBase},
    {"urn:ietf:params:xml:ns:yang:ietf-pim-bidir", YangDataType::IetfPimBidir},
    {"urn:ietf:params:xml:ns:yang:ietf-pim-dm", YangDataType::IetfPimDm},
    {"urn:ietf:params:xml:ns:yang:ietf-pim-rp", YangDataType::IetfPimRp},
    {"urn:ietf:params:xml:ns:yang:ietf-pim-sm", YangDataType::IetfPimSm},
    {"urn:ietf:params:xml:ns:yang:ietf-ptp", YangDataType::IetfPtp},
    {"urn:ietf:params:xml:ns:yang:ietf-restconf-monitoring", YangDataType::IetfRestconfMonitoring},
    {"urn:ietf:params:xml:ns:yang:ietf-restconf-subscribed-notifications", YangDataType::IetfRestconfSubscribedNotifications},
    {"urn:ietf:params:xml:ns:yang:ietf-restconf", YangDataType::IetfRestconf},
    {"urn:ietf:params:xml:ns:yang:ietf-rib-extension", YangDataType::IetfRibExtension},
    {"urn:ietf:params:xml:ns:yang:ietf-rift", YangDataType::IetfRift},
    {"urn:ietf:params:xml:ns:yang:ietf-rip", YangDataType::IetfRip},
    {"urn:ietf:params:xml:ns:yang:ietf-routing-policy", YangDataType::IetfRoutingPolicy},
    {"urn:ietf:params:xml:ns:yang:ietf-routing-types", YangDataType::IetfRoutingTypes},
    {"urn:ietf:params:xml:ns:yang:ietf-routing", YangDataType::IetfRouting},
    {"urn:ietf:params:xml:ns:yang:ietf-sap-ntw", YangDataType::IetfSapNtw},
    {"urn:ietf:params:xml:ns:yang:ietf-schc-compound-ack", YangDataType::IetfSchcCompoundAck},
    {"urn:ietf:params:xml:ns:yang:ietf-schc", YangDataType::IetfSchc},
    {"urn:ietf:params:xml:ns:yang:ietf-segment-routing-common", YangDataType::IetfSegmentRoutingCommon},
    {"urn:ietf:params:xml:ns:yang:ietf-segment-routing-mpls", YangDataType::IetfSegmentRoutingMpls},
    {"urn:ietf:params:xml:ns:yang:ietf-segment-routing", YangDataType::IetfSegmentRouting},
    {"urn:ietf:params:xml:ns:yang:ietf-service-assurance-device", YangDataType::IetfServiceAssuranceDevice},
    {"urn:ietf:params:xml:ns:yang:ietf-service-assurance-interface", YangDataType::IetfServiceAssuranceInterface},
    {"urn:ietf:params:xml:ns:yang:ietf-service-assurance", YangDataType::IetfServiceAssurance},
    {"urn:ietf:params:xml:ns:yang:ietf-sid-file", YangDataType::IetfSidFile},
    {"urn:ietf:params:xml:ns:yang:ietf-snmp-common", YangDataType::IetfSnmpCommon},
    {"urn:ietf:params:xml:ns:yang:ietf-snmp-community", YangDataType::IetfSnmpCommunity},
    {"urn:ietf:params:xml:ns:yang:ietf-snmp-engine", YangDataType::IetfSnmpEngine},
    {"urn:ietf:params:xml:ns:yang:ietf-snmp-notification", YangDataType::IetfSnmpNotification},
    {"urn:ietf:params:xml:ns:yang:ietf-snmp-proxy", YangDataType::IetfSnmpProxy},
    {"urn:ietf:params:xml:ns:yang:ietf-snmp-ssh", YangDataType::IetfSnmpSsh},
    {"urn:ietf:params:xml:ns:yang:ietf-snmp-target", YangDataType::IetfSnmpTarget},
    {"urn:ietf:params:xml:ns:yang:ietf-snmp-tls", YangDataType::IetfSnmpTls},
    {"urn:ietf:params:xml:ns:yang:ietf-snmp-tsm", YangDataType::IetfSnmpTsm},
    {"urn:ietf:params:xml:ns:yang:ietf-snmp-usm", YangDataType::IetfSnmpUsm},
    {"urn:ietf:params:xml:ns:yang:ietf-snmp-vacm", YangDataType::IetfSnmpVacm},
    {"urn:ietf:params:xml:ns:yang:ietf-snmp", YangDataType::IetfSnmp},
    {"urn:ietf:params:xml:ns:yang:ietf-softwire-br", YangDataType::IetfSoftwireBr},
    {"urn:ietf:params:xml:ns:yang:ietf-softwire-ce", YangDataType::IetfSoftwireCe},
    {"urn:ietf:params:xml:ns:yang:ietf-softwire-common", YangDataType::IetfSoftwireCommon},
    {"urn:ietf:params:xml:ns:yang:ietf-ssh-client", YangDataType::IetfSshClient},
    {"urn:ietf:params:xml:ns:yang:ietf-ssh-common", YangDataType::IetfSshCommon},
    {"urn:ietf:params:xml:ns:yang:ietf-ssh-server", YangDataType::IetfSshServer},
    {"urn:ietf:params:xml:ns:yang:ietf-subscribed-notifications", YangDataType::IetfSubscribedNotifications},
    {"urn:ietf:params:xml:ns:yang:ietf-syslog", YangDataType::IetfSyslog},
    {"urn:ietf:params:xml:ns:yang:ietf-system-capabilities", YangDataType::IetfSystemCapabilities},
    {"urn:ietf:params:xml:ns:yang:ietf-system-tacacs-plus", YangDataType::IetfSystemTacacsPlus},
    {"urn:ietf:params:xml:ns:yang:ietf-system", YangDataType::IetfSystem},
    {"urn:ietf:params:xml:ns:yang:ietf-sztp-bootstrap-server", YangDataType::IetfSztpBootstrapServer},
    {"urn:ietf:params:xml:ns:yang:ietf-sztp-conveyed-info", YangDataType::IetfSztpConveyedInfo},
    {"urn:ietf:params:xml:ns:yang:ietf-sztp-csr", YangDataType::IetfSztpCsr},
    {"urn:ietf:params:xml:ns:yang:ietf-tcg-algs", YangDataType::IetfTcgAlgs},
    {"urn:ietf:params:xml:ns:yang:ietf-tcp-client", YangDataType::IetfTcpClient},
    {"urn:ietf:params:xml:ns:yang:ietf-tcp-common", YangDataType::IetfTcpCommon},
    {"urn:ietf:params:xml:ns:yang:ietf-tcp-server", YangDataType::IetfTcpServer},
    {"urn:ietf:params:xml:ns:yang:ietf-tcp", YangDataType::IetfTcp},
    {"urn:ietf:params:xml:ns:yang:ietf-te-packet-types", YangDataType::IetfTePacketTypes},
    {"urn:ietf:params:xml:ns:yang:ietf-te-topology-state", YangDataType::IetfTeTopologyState},
    {"urn:ietf:params:xml:ns:yang:ietf-te-topology", YangDataType::IetfTeTopology},
    {"urn:ietf:params:xml:ns:yang:ietf-te-types", YangDataType::IetfTeTypes},
    {"urn:ietf:params:xml:ns:yang:ietf-template", YangDataType::IetfTemplate},
    {"urn:ietf:params:xml:ns:yang:ietf-tls-client", YangDataType::IetfTlsClient},
    {"urn:ietf:params:xml:ns:yang:ietf-tls-common", YangDataType::IetfTlsCommon},
    {"urn:ietf:params:xml:ns:yang:ietf-tls-server", YangDataType::IetfTlsServer},
    {"urn:ietf:params:xml:ns:yang:ietf-tpm-remote-attestation", YangDataType::IetfTpmRemoteAttestation},
    {"urn:ietf:params:xml:ns:yang:ietf-truststore", YangDataType::IetfTruststore},
    {"urn:ietf:params:xml:ns:yang:ietf-twamp", YangDataType::IetfTwamp},
    {"urn:ietf:params:xml:ns:yang:ietf-vn", YangDataType::IetfVn},
    {"urn:ietf:params:xml:ns:yang:ietf-voucher-request", YangDataType::IetfVoucherRequest},
    {"urn:ietf:params:xml:ns:yang:ietf-voucher", YangDataType::IetfVoucher},
    {"urn:ietf:params:xml:ns:yang:ietf-vpn-common", YangDataType::IetfVpnCommon},
    {"urn:ietf:params:xml:ns:yang:ietf-vrrp", YangDataType::IetfVrrp},
    {"urn:ietf:params:xml:ns:yang:ietf-wson-topology", YangDataType::IetfWsonTopology},
    {"urn:ietf:params:xml:ns:yang:ietf-yang-instance-data", YangDataType::IetfYangInstanceData},
    {"urn:ietf:params:xml:ns:yang:ietf-yang-library", YangDataType::IetfYangLibrary},
    {"urn:ietf:params:xml:ns:yang:ietf-yang-metadata", YangDataType::IetfYangMetadata},
    {"urn:ietf:params:xml:ns:yang:ietf-yang-patch", YangDataType::IetfYangPatch},
    {"urn:ietf:params:xml:ns:yang:ietf-yang-push", YangDataType::IetfYangPush},
    {"urn:ietf:params:xml:ns:yang:ietf-yang-schema-mount", YangDataType::IetfYangSchemaMount},
    {"urn:ietf:params:xml:ns:yang:ietf-yang-structure-ext", YangDataType::IetfYangStructureExt},
    {"urn:ietf:params:xml:ns:yang:ietf-yang-types", YangDataType::IetfYangTypes},
    {"urn:ietf:params:xml:ns:yang:ietf-ztp-types", YangDataType::IetfZtpTypes}
  };

  auto it = m.find(ns);
  if (it != m.end())
    return it->second;

  // Fallback: check suffix after last ':' for module name (hyphen or underscore).
  auto pos = ns.find_last_of(':');
  if (pos != string::npos && pos + 1 < ns.size()) {
    string suffix = ns.substr(pos + 1);
    // try direct match with hyphen form
    for (const auto &p : m) {
      auto urn = p.first;
      if (urn.size() >= suffix.size() && urn.compare(urn.size() - suffix.size(), suffix.size(), suffix) == 0)
        return p.second;
    }
  }

  return YangDataType::Unknown;
}

} // namespace netcli
