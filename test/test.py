import subprocess
from functools import wraps
from typing import List


def log_func(f):
    @wraps(f)
    def wrapper(*args, **kwargs):
        print(f"### Enter Func: {f.__name__}")
        res = f(*args, **kwargs)
        print(f"--- Exit Func: {f.__name__}\n")
        return res

    return wrapper


def run_sql_commands(commands: List[str]) -> List[str]:
    """在给定的进程上运行多个 SQL 命令并返回输出"""
    process = subprocess.Popen(["./my_program"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)
    commands = "\n".join(commands)
    commands += "\n"
    output, _ = process.communicate(input=commands)
    return list(filter(lambda x: x != "", map(str.strip, output.split("\n"))))


@log_func
def test_database_operations():
    """测试数据库操作"""
    commands = [
        "insert 1 yan gmail",
        "insert 2 fei qq",
        "foo bar",
        "select",
        ".exit",
    ]
    output = run_sql_commands(commands)
    # assert "Alice" in output, "Data operation failed"
    expect = [
        "db > Executed.",
        "db > Executed.",
        "db > Unrecognized keyword at start of 'foo bar'.",
        "db > 1 yan gmail",
        "2 fei qq",
        "Executed.",
        "db >",
    ]
    # print("---print output")
    # for s in output:
    #     print(s)
    # print("---end output")
    # print("---print expect")
    # for s in expect:
    #     print(s)
    # print("---end expect")
    print(output)
    assert output == expect


@log_func
def test_database_pressure():
    commands = [f"insert {i} user{i} user#{i}@email.com" for i in range(1041)]
    commands.append(".exit")
    output = run_sql_commands(commands)
    print(output[-3:])
    assert output[-2] == "db > Error: Table full."


@log_func
def test_database_long_string():
    name = "a" * 32
    email = "e" * 255
    commands = [f"insert 1 {name} {email}",
                "select", ".exit"]
    output = run_sql_commands(commands)
    expect = ['db > Executed.',
              'db > 1 aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa '
              'eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee',
              'Executed.', 'db >']
    print(output)
    assert output == expect


@log_func
def test_database_too_long_string():
    name = "a" * 33
    email = "e" * 256
    commands = [f"insert 1 {name} {email}",
                "select"]
    output = run_sql_commands(commands)
    print(output)


if __name__ == "__main__":
    test_database_operations()
    test_database_pressure()
    test_database_long_string()
    test_database_too_long_string()
