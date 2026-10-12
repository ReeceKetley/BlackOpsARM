"""Independent typed acceptance for genuine startup-service integration traces."""
import json
import struct

COMMANDS = ['wait', 'vstr', 'exec_addtext', 'exec', 'cmdlist', 'restoreDvars',
            'setu', 'dvar_color', 'dvar_float', 'dvar_int', 'dvar_bool', 'dvardump',
            'dvarlist', 'reset', 'setdvartotime', 'setfromlocString', 'setfromdvar',
            'setadmindvar', 'seta', 'sets', 'set', 'togglep', 'toggle']


def bits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def validate(stdout, width):
    rows = [json.loads(line) for line in stdout.splitlines()]
    groups = {}
    for row in rows:
        if type(row) is not dict or len(row) != 1:
            raise RuntimeError('One typed record required per line')
        name, value = next(iter(row.items()))
        if name not in ('startup_command_node', 'startup_vector', 'startup_ownership', 'startup_services') or type(value) is not dict:
            raise RuntimeError('Unexpected startup trace')
        groups.setdefault(name, []).append(value)
    nodes = groups.get('startup_command_node', [])
    expected_nodes = [dict(index=index, name=name, callback_match=True) for index, name in enumerate(COMMANDS)]
    if nodes != expected_nodes or any(type(row['index']) is not int or row['callback_match'] is not True for row in nodes):
        raise RuntimeError('Actual command order/identity trace differs')
    vectors = groups.get('startup_vector', [])
    if len(vectors) != 9:
        raise RuntimeError('Nine actual vector transitions required')
    original = list(map(bits, (-1.25, 2.5, -3.75)))
    updated = list(map(bits, (4.125, -5.5, 6.75)))
    for index, row in enumerate(vectors):
        if type(row.get('index')) is not int or row['index'] != index or type(row.get('type')) is not int or row['type'] != (3, 10, 11)[index // 3]:
            raise RuntimeError('Vector type/index differs')
        for name in ('current', 'latched', 'reset'):
            values = row.get(name)
            if type(values) is not list or len(values) not in (3, 4) or any(type(value) is not int for value in values):
                raise RuntimeError('Exact float bit records required')
            expected = original if name == 'reset' or index % 3 != 1 else updated
            if values[:3] != expected:
                raise RuntimeError('Actual vector bits differ from independent model')
        for name in ('current_tail', 'latched_tail'):
            expected_tail = (0x67452301 if name == 'current_tail' else 0xefcdab89) + index // 3
            if type(row.get(name)) is not int or row[name] != expected_tail:
                raise RuntimeError('Neighboring vector bytes changed')
    ownership = groups.get('startup_ownership', [])
    if len(ownership) != 9 or [row.get('stage') for row in ownership] != list(range(9)):
        raise RuntimeError('All ownership transitions required')
    fields = ('server_refs', 'client_refs', 'server_allocations', 'server_buckets', 'client_allocations', 'client_buckets')
    for row in ownership:
        if set(row) != set(fields) | {'stage'} or any(type(value) is not int or value < 0 for value in row.values()):
            raise RuntimeError('Ownership counters must be nonnegative integers')
    if any(ownership[stage][field] for stage in (0, 8) for field in fields):
        raise RuntimeError('Real SL/MT baseline not restored')
    if any(ownership[1][field] != ownership[2][field] for field in fields):
        raise RuntimeError('Equal-value assignment changed ownership')
    summaries = groups.get('startup_services', [])
    if len(summaries) != 1:
        raise RuntimeError('Exactly one integration summary required')
    summary = summaries[0]
    fixed = dict(pointer_bytes=width, initial_dvars=2, commands_before_cmd_init=18,
                 commands_after_cmd_init=23, actual_parse=True, actual_sl_instances=2,
                 actual_loader_dvar=True, com_init_executed=False)
    if set(summary) != set(fixed) | {'actual_command_operations'}:
        raise RuntimeError('Startup integration scope differs')
    for key, expected in fixed.items():
        if type(summary[key]) is not type(expected) or summary[key] != expected:
            raise RuntimeError('Startup summary differs: ' + key)
    if type(summary['actual_command_operations']) is not int or summary['actual_command_operations'] < 10:
        raise RuntimeError('Actual command operation coverage missing')
    if len(rows) != 42 or next(iter(rows[-1])) != 'startup_ownership' or rows[-1]['startup_ownership']['stage'] != 8:
        raise RuntimeError('Final successful ownership trace required')
    return rows
