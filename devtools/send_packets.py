#!/usr/bin/env python3

import argparse

from scapy.all import Ether, Raw, sendp

DST_MAC = "ff:ff:ff:ff:ff:ff"

parser = argparse.ArgumentParser(description="Send test packets to the PMD's device")
parser.add_argument("iface", help="host interface connected to the PMD's device (e.g. tap0)")
parser.add_argument("count", type=int, help="number of packets to send")
args = parser.parse_args()

for seq in range(args.count):
    pkt = Ether(dst=DST_MAC, type=0x88B5) / Raw(f"hello pmd #{seq}".encode().ljust(46, b"\x00"))
    sendp(pkt, iface=args.iface, verbose=False)
    print(f"sent #{seq}, len={len(pkt)}")
