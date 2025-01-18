import json

import mlc


def test_json_loads_bool() -> None:
    src = json.dumps([True, False])
    result = mlc.json_loads(src)
    assert isinstance(result, mlc.List) and len(result) == 2
    assert result[0] == True
    assert result[1] == False


def test_json_loads_none() -> None:
    src = json.dumps([None])
    result = mlc.json_loads(src)
    assert isinstance(result, mlc.List) and len(result) == 1
    assert result[0] is None
