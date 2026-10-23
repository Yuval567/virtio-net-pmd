#!/usr/bin/env python3

import argparse

from scapy.all import Ether, Raw, sendp

SRC_MAC = "BB:BB:BB:BB:BB:BB"
DST_MAC = "FF:FF:FF:FF:FF:FF"

parser = argparse.ArgumentParser(description="Send test packets to the PMD's device")
parser.add_argument("iface", help="host interface connected to the PMD's device (e.g. tap0)")
parser.add_argument("count", type=int, help="number of packets to send")
args = parser.parse_args()

payload = b"\xAA" * 1500
pkt = Ether(src=SRC_MAC, dst=DST_MAC, type=0x05DC) / Raw(payload)

sendp(pkt, iface=args.iface, verbose=False, count=args.count)