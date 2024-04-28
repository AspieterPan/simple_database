import os
import subprocess
from functools import wraps
from typing import List, Callable


def log_func(f: Callable) -> Callable:
    @wraps(f)
    def wrapper(*args, **kwargs):
        print(f"### Enter Func: {f.__name__}")
        res = f(*args, **kwargs)
        print(f"--- Exit Func: {f.__name__}\n")
        return res

    return wrapper


def db_manager(f: Callable) -> Callable:
    """ 用于修饰数据库测试函数的装饰器

    开始测试前， 清除旧的文件

    测试后， 清除测试文件

    p.s 只能用于装饰第一个参数是数据库文件的测试函数
    """

    @wraps(f)
    def wrapper(*args, **kwargs):
        if os.path.exists(args[0]):
            os.remove(args[0])
        res = f(*args, **kwargs)
        os.remove(args[0])

    return wrapper


def run_sql_commands(dbname: str, commands: List[str]) -> List[str]:
    """在给定的进程上运行多个 SQL 命令并返回输出
    :param dbname:
    :param commands:
    """
    process = subprocess.Popen(["./my_program", "./db/test.db"], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                               text=True)
    commands = "\n".join(commands)
    commands += "\n"
    output, _ = process.communicate(input=commands)
    return list(filter(lambda x: x != "", map(str.strip, output.split("\n"))))


@log_func
@db_manager
def test_database_operations(dbname: str):
    """ 测试数据库操作 """
    commands = [
        "insert 1 yan gmail",
        "insert 2 fei qq",
        "foo bar",
        "select",
        ".exit",
    ]

    output = run_sql_commands(dbname, commands)
    print(output)

    expect = [
        "db > Executed.",
        "db > Executed.",
        "db > Unrecognized keyword at start of 'foo bar'.",
        "db > 1 yan gmail",
        "2 fei qq",
        "Executed.",
        "db >",
    ]
    assert output == expect


@log_func
@db_manager
def test_database_pressure(dbname):
    commands = [f"insert {i} user{i} user#{i}@email.com" for i in range(1441)]
    commands.append(".exit")

    output = run_sql_commands(dbname, commands)
    print(output[-3:])

    assert output[-2] == "db > Error: Table full."


@log_func
@db_manager
def test_database_long_string(dbname):
    name = "a" * 32
    email = "e" * 255
    commands = [f"insert 1 {name} {email}",
                "select", ".exit"]

    output = run_sql_commands(dbname, commands)
    print(output)

    expect = ['db > Executed.',
              'db > 1 aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa '
              'eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee',
              'Executed.', 'db >']
    assert output == expect


@log_func
@db_manager
def test_database_too_long_string(dbname):
    name = "a" * 33
    email = "e" * 256
    commands = [f"insert 1 {name} {email}",
                "select"]

    output = run_sql_commands(dbname, commands)
    print(output)

    expect = ['db > String is too long', 'db > Executed.', 'db > Error reading input']
    assert output == expect


@log_func
@db_manager
def test_database_persistence(dbname):
    """测试数据库存储功能"""
    # insert datas and store db
    commands = ["insert 1 yan yyy",
                "insert 2 liu lll",
                "insert 3 tao ttt",
                "insert 4 fei fff",
                ".exit"]
    run_sql_commands(dbname, commands)

    # reopen the db
    commands = ["select", ".exit"]
    output = run_sql_commands(dbname, commands)
    print(output)
    expect = ['db > 1 yan yyy', '2 liu lll', '3 tao ttt', '4 fei fff', 'Executed.', 'db >']
    assert output == expect


@log_func
def test_database_persistence_pressure(dbname):
    """数据库存储功能压力测试"""
    commands = [f"insert {i} user{i} email{i}" for i in range(999)]
    commands.append(".exit")
    run_sql_commands(dbname, commands)

    output = run_sql_commands(dbname, ["select", ".exit"])
    print(output[-50:])


if __name__ == "__main__":
    file_name = "./db/test.db"
    test_database_operations(file_name)
    test_database_pressure(file_name)
    test_database_long_string(file_name)
    test_database_too_long_string(file_name)
    test_database_persistence(file_name)
    test_database_persistence_pressure(file_name)
