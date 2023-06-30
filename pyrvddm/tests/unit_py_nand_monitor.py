import unittest as ut

import sys

sys.path.append('..')

from pyrvddm.nand_monitor import NandFlashMonitor


class TestNandFlashMonitor(ut.TestCase):
    def setUp(self):
        pass

    def test_read_valid(self):
        monitor = NandFlashMonitor()
        events = [
            ('InitialRead', {'address': 0x2000}),
            ('SetAddress', {'Block': 2, 'Page': 0, 'Byte': 5}),
            ('DoRead', {}),
            ('WaitForReady', {'time': 200}),
            ('DoTransfer', {}),
            ('DoRead', {}),
            ('WaitForReady', {'time': 200}),
            ('DoTransfer', {}),
            ('EndTransaction', {}),
        ]
        monitor.process_events(events)
        self.assertEqual(monitor.state, 'idle')
        self.assertEqual(monitor.memory.trace, [('write', 0x2000, bytes(1)), ('write', 0x2001, bytes(1))])

    def test_program_valid(self):
        monitor = NandFlashMonitor()
        events = [
            ('InitialProgram', {'address': 0x2000}),
            ('SetAddress', {'Block': 2, 'Page': 0, 'Byte': 5}),
            ('DoProgram', {}),
            ('WaitForReady', {'time': 700}),
            ('DoTransfer', {}),
            ('DoProgram', {}),
            ('WaitForReady', {'time': 700}),
            ('DoTransfer', {}),
            ('EndTransaction', {}),
        ]
        monitor.process_events(events)
        self.assertEqual(monitor.state, 'idle')
        self.assertEqual(monitor.memory.trace, [('read', 0x2000, 1), ('read', 0x2001, 1)])

    def test_erase_valid(self):
        monitor = NandFlashMonitor()
        events = [
            ('InitialErase', {}),
            ('SetAddress', {'Block': 2}),
            ('DoErase', {}),
            ('WaitForReady', {'time': 3000}),
            ('DoErase', {}),
            ('WaitForReady', {'time': 3000}),
            ('EndTransaction', {}),
        ]
        monitor.process_events(events)
        self.assertEqual(monitor.state, 'idle')

    def test_read_timeout(self):
        monitor = NandFlashMonitor()
        events = [
            ('InitialRead', {'address': 0x2000}),
            ('SetAddress', {'Block': 2, 'Page': 0, 'Byte': 5}),
            ('DoRead', {}),
            ('WaitForReady', {'time': 1}),
            ('DoTransfer', {}),
        ]
        monitor.process_events(events)
        self.assertEqual(monitor.state, 'fault')
