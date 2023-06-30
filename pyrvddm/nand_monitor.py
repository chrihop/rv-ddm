#!/usr/bin/env python3
from typing import Sequence, Tuple, Dict, List, Any
import time
from pathlib import Path
import logging
import sys

from transitions import Event, EventData
from transitions.extensions.nesting import HierarchicalMachine as Machine
from transitions.extensions.nesting import NestedState as State
from transitions.extensions.diagrams import \
    HierarchicalGraphMachine as GraphMachine
from enum import Enum, auto


logging.basicConfig(level=logging.DEBUG, handlers=[logging.StreamHandler(sys.stdout)])
log = logging.getLogger(__name__)


class NandAddrType(Enum):
    Block = auto()
    Page = auto()
    Byte = auto()


class NandTransaction(Enum):
    Read = auto()
    Program = auto()
    Erase = auto()
    Idle = auto()

    @staticmethod
    def from_event(event: Event):
        return {
            'InitialRead': NandTransaction.Read,
            'InitialProgram': NandTransaction.Program,
            'InitialErase': NandTransaction.Erase,
        }.get(event.name, NandTransaction.Idle)


class NandFlash:
    Spec = {
        'n_blocks': 1024,
        'n_pages': 64,
        'n_bytes': 2048,
        'time_read_page': 110,
        'time_program_page': 660,
        'time_erase_block': 2200,
        'time_reset': 550,
    }

    def __init__(self):
        self.cursor: List[int, int, int] = [-1, -1, -1]
        self.operation_mode = NandTransaction.Idle
        self.previous_action = None
        self.action_finish = -1

    def __str__(self):
        return f'Cursor=(Block={self.cursor[0]}, Page={self.cursor[1]}, Byte={self.cursor[2]}), ' \
               f'Operation={self.operation_mode.name}, ' \
               f'PreviousAction={self.previous_action}, ' \
               f'ActionFinishAt={self.action_finish}'

    def set_address(self, addr_type: NandAddrType, addr: int) -> bool:
        if addr_type == NandAddrType.Block and addr < NandFlash.Spec[
            'n_blocks']:
            self.cursor[0] = addr
            return True
        elif addr_type == NandAddrType.Page and addr < NandFlash.Spec[
            'n_pages']:
            self.cursor[1] = addr
            return True
        elif addr_type == NandAddrType.Byte and addr < NandFlash.Spec[
            'n_bytes']:
            self.cursor[2] = addr
            return True
        else:
            return False

    def check_address(self):
        if self.operation_mode == NandTransaction.Read:
            return all(x >= 0 for x in self.cursor)
        elif self.operation_mode == NandTransaction.Program:
            return all(x >= 0 for x in self.cursor)
        elif self.operation_mode == NandTransaction.Erase:
            return self.cursor[0] >= 0 and all(x == -1 for x in self.cursor[1:])
        else:
            return False

    def start_operation(self, operation: NandTransaction) -> bool:
        if self.operation_mode == NandTransaction.Idle:
            self.operation_mode = operation
            self.cursor = [-1, -1, -1]
            return True
        else:
            return False

    def finish_operation(self) -> bool:
        self.operation_mode = NandTransaction.Idle
        return True

    def launch_action(self, action: str):
        if action == 'DoRead' and self.operation_mode == NandTransaction.Read:
            self.previous_action = action
            self.action_finish = time.time_ns() + NandFlash.Spec[
                'time_read_page'] * 1000000
            return True
        elif action == 'DoProgram' and self.operation_mode == NandTransaction.Program:
            self.previous_action = action
            self.action_finish = time.time_ns() + NandFlash.Spec[
                'time_program_page'] * 1000000
            return True
        elif action == 'DoErase' and self.operation_mode == NandTransaction.Erase:
            self.previous_action = action
            self.action_finish = time.time_ns() + NandFlash.Spec[
                'time_erase_block'] * 1000000
            return True
        elif action == 'EndTransaction' and self.operation_mode != NandTransaction.Idle:
            self.previous_action = None
            self.operation_mode = NandTransaction.Idle
            return True
        else:
            return False

    def cursor_increment_byte(self, x):
        block, page, byte = self.cursor
        byte += x
        inc_page = byte // NandFlash.Spec['n_bytes']
        byte %= NandFlash.Spec['n_bytes']
        page += inc_page
        inc_block = page // NandFlash.Spec['n_pages']
        page %= NandFlash.Spec['n_pages']
        block += inc_block
        self.cursor = (block, page, byte)

    def cursor_increment_block(self, x):
        block, page, byte = self.cursor
        block += x
        self.cursor = (block, page, byte)

    def finalize_action(self, action: str = None) -> bool:
        if self.previous_action in {'DoRead', 'DoProgram'} and action == 'DoTransfer':
            self.cursor_increment_byte(1)
        elif self.previous_action == 'DoErase' and \
                (action is None or action == 'DoErase'):
            self.cursor_increment_block(1)
        else:
            return False
        return self.check_address()

    def check_ready(self) -> bool:
        return self.action_finish <= time.time_ns()


class MemoryModel:
    Spec = {
        'low_bound': 0,
        'high_bound': 0x100000,
    }

    def __init__(self, low_bound=Spec['low_bound'],
                 high_bound=Spec['high_bound']):
        self.low_bound = low_bound
        self.high_bound = high_bound
        self.trace = []
        pass

    def readable(self, addr: int):
        return self.low_bound <= addr < self.high_bound

    def writable(self, addr: int):
        return self.low_bound <= addr < self.high_bound

    def read(self, addr: int, size: int) -> bytes:
        self.trace.append(('read', addr, size))

    def write(self, addr: int, data: bytes):
        self.trace.append(('write', addr, data))


class NandFlashMonitor:
    def __init__(self):
        self.states = [
            'idle', 'fault', 'recovery',
            {
                'name': 'nandRead',
                'children': [
                    'waitForAddress',
                    'waitForAction',
                    'internalTransfer',
                ],
                'initial': 'waitForAddress',
                'transitions': [
                    {'trigger': 'SetAddress', 'source': 'waitForAddress',
                     'dest': 'waitForAction', 'conditions': 'on_set_address'},
                    {'trigger': 'DoRead', 'source': 'waitForAction',
                     'dest': 'internalTransfer', 'after': 'start_clock'},
                    {'trigger': 'WaitForReady', 'source': 'internalTransfer',
                     'dest': '=', 'after': 'start_timer'},
                    {'trigger': 'DoTransfer', 'source': 'internalTransfer',
                     'dest': 'waitForAction', 'conditions': 'check_ready',
                     'after': 'on_transfer'},
                ],
                'on_enter': 'start_transaction',
            },
            {
                'name': 'nandProgram',
                'children': [
                    'waitForAddress',
                    'waitForAction',
                    'internalTransfer',
                ],
                'initial': 'waitForAddress',
                'transitions': [
                    {'trigger': 'SetAddress', 'source': 'waitForAddress',
                     'dest': 'waitForAction', 'conditions': 'on_set_address'},
                    {'trigger': 'DoProgram', 'source': 'waitForAction',
                     'dest': 'internalTransfer', 'after': 'start_clock'},
                    {'trigger': 'WaitForReady', 'source': 'internalTransfer',
                     'dest': '=', 'after': 'start_timer'},
                    {'trigger': 'DoTransfer', 'source': 'internalTransfer',
                     'dest': 'waitForAction', 'conditions': 'check_ready',
                     'after': 'on_transfer'},
                ],
                'on_enter': 'start_transaction',
            },
            {
                'name': 'nandErase',
                'children': [
                    'waitForAddress',
                    'waitForAction',
                    'internalErase',
                ],
                'initial': 'waitForAddress',
                'transitions': [
                    {'trigger': 'SetAddress', 'source': 'waitForAddress',
                     'dest': 'waitForAction', 'conditions': 'on_set_address'},
                    {'trigger': 'DoErase',
                     'source': ['waitForAction', 'internalErase'],
                     'dest': 'internalErase', 'conditions': 'check_ready',
                     'after': ['start_clock', 'on_erase']},
                    {'trigger': 'WaitForReady', 'source': 'internalErase',
                     'dest': '=', 'after': 'start_timer'},
                ],
                'on_enter': 'start_transaction',
            },
        ]

        self.transitions = [
            {'trigger': 'InitialRead', 'source': 'idle', 'dest': 'nandRead',
             'conditions': 'on_buffer_permission'},
            {'trigger': 'InitialProgram', 'source': 'idle',
             'dest': 'nandProgram', 'conditions': 'on_buffer_permission'},
            {'trigger': 'InitialErase', 'source': 'idle', 'dest': 'nandErase'},
            {'trigger': 'EndTransaction',
             'source': ['nandRead_waitForAction', 'nandProgram_waitForAction',
                        'nandErase_waitForAction', 'nandErase_internalErase'],
             'dest': 'idle',
             'conditions': 'on_finish'},
            {'trigger': 'error', 'source': '*', 'dest': 'fault'},
            {'trigger': 'recover', 'source': 'fault', 'dest': 'recovery',
             'after': 'on_recover'},
            {'trigger': 'recover_succ', 'source': 'recovery', 'dest': 'idle'},
        ]

        self.machine = Machine(
            model=self,
            states=self.states,
            transitions=self.transitions,
            initial='idle',
            send_event=True,
        )

        self.nand = NandFlash()
        self.memory = MemoryModel()
        self.current_event: Event = None
        self.buffer = -1
        self.error_reason = []

    def on_event(self, event: Tuple[str, Dict[str, Any]]) -> bool:
        self.current_event = event
        print(f'Exec {event}')
        name, args = event
        succ = self.machine.trigger_event(self, name, **args)
        return succ

    def process_events(self, events: Sequence[Tuple[str, Dict[str, Any]]]):
        for e in events:
            if self.on_event(e) is False:
                print(f'Event {e} transition failed!')
                self.error(f'Transition of event{e} from state {self.state} failed.')

    def start_transaction(self, e: EventData):
        if self.nand.operation_mode != NandTransaction.Idle:
            self.error('NandFlash is busy!')
            return
        self.nand.start_operation(NandTransaction.from_event(e.event))

    def on_buffer_permission(self, e: EventData) -> bool:
        address = e.kwargs['address']
        if e.event.name == 'InitialRead' and self.memory.readable(address):
            self.buffer = address
            return True
        elif e.event.name == 'InitialProgram' and self.memory.writable(address):
            self.buffer = address
            return True
        else:
            return False

    def on_set_address(self, event) -> bool:
        for k, v in event.kwargs.items():
            self.nand.set_address(NandAddrType[k], v)
        return self.nand.check_address()

    def start_clock(self, e: EventData):
        self.nand.launch_action(e.event.name)

    def start_timer(self, e: EventData):
        time.sleep(e.kwargs['time'] / 1000)

    def check_ready(self, e: EventData) -> bool:
        return self.nand.check_ready()

    def on_transfer(self, e: EventData):
        if self.nand.finalize_action(e.event.name):
            if self.nand.operation_mode == NandTransaction.Read and self.memory.writable(
                    self.buffer):
                self.memory.write(self.buffer, bytes(1))
                self.buffer += 1
            elif self.nand.operation_mode == NandTransaction.Program and self.memory.readable(
                    self.buffer):
                self.memory.read(self.buffer, 1)
                self.buffer += 1
            else:
                self.error('Buffer permission error!')
        else:
            self.error('NandFlash cursor error!')

    def on_erase(self, e: EventData):
        if self.nand.finalize_action(e.event.name) is False:
            self.error(f'NandFlash cursor error! cursor = {self.nand.cursor}')

    def on_finish(self, e: EventData) -> bool:
        if self.nand.operation_mode != NandTransaction.Idle:
            self.nand.finish_operation()
            self.buffer = -1
            return True
        else:
            return False

    def error(self, reason: str = ''):
        print(f'{reason}')
        self.error_reason = reason
        self.to_fault()

    def draw(self, figure: Path = Path('nand.png')):
        GraphMachine(
            model=self,
            states=self.states,
            transitions=self.transitions,
            initial='idle',
            send_event=True,
        )

        g = self.get_graph()
        g.draw(figure.open('wb'), prog='dot', format='png')


def main():
    monitor = NandFlashMonitor()
    monitor.draw()
    events = [
        ('InitialRead', {'address': 0x1000}),
        ('SetAddress', {'Block': 2, 'Page': 3, 'Byte': 5}),
        ('DoRead', {}),
        ('WaitForReady', {'time': 200}),
        ('DoTransfer', {}),
        ('DoRead', {}),
        ('WaitForReady', {'time': 200}),
        ('DoTransfer', {}),
        ('EndTransaction', {}),
    ]
    monitor.process_events(events)
    print(f'Memory: {monitor.memory.trace}')
    print(f'NandFlash: {monitor.nand}')
    print(f'State: {monitor.state}')


if __name__ == '__main__':
    main()
