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


def db_context_manage(f: Callable) -> Callable:
    """用于修饰数据库测试函数的装饰器

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
    process = subprocess.Popen(
        ["./simple_db", "./db/test.db"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        text=True,
    )
    commands = "\n".join(commands)
    commands += "\n"
    output, _ = process.communicate(input=commands)
    return list(filter(lambda x: x != "", map(str.strip, output.split("\n"))))


@log_func
@db_context_manage
def test_database_operations(dbname: str):
    """测试数据库操作"""
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
@db_context_manage
def test_database_pressure(dbname):
    commands = [f"insert {i} user{i} user#{i}@email.com" for i in range(1441)]
    commands.append(".exit")

    output = run_sql_commands(dbname, commands)
    print(output[-3:])

    assert output[-2] == "db > Error: Table full."


@log_func
@db_context_manage
def test_database_long_string(dbname):
    name = "a" * 32
    email = "e" * 255
    commands = [f"insert 1 {name} {email}", "select", ".exit"]

    output = run_sql_commands(dbname, commands)
    print(output)

    expect = [
        "db > Executed.",
        "db > 1 aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa "
        "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
        "Executed.",
        "db >",
    ]
    assert output == expect


@log_func
@db_context_manage
def test_database_too_long_string(dbname):
    name = "a" * 33
    email = "e" * 256
    commands = [f"insert 1 {name} {email}", "select"]

    output = run_sql_commands(dbname, commands)
    print(output)

    expect = ["db > String is too long", "db > Executed.", "db > Error reading input"]
    assert output == expect


@log_func
@db_context_manage
def test_database_persistence(dbname):
    """测试数据库存储功能"""
    # insert datas and store db
    commands = [
        "insert 1 yan yyy",
        "insert 2 liu lll",
        "insert 3 tao ttt",
        "insert 4 fei fff",
        ".exit",
    ]
    run_sql_commands(dbname, commands)

    # reopen the db
    commands = ["select", ".exit"]
    output = run_sql_commands(dbname, commands)
    print(output)
    expect = [
        "db > 1 yan yyy",
        "2 liu lll",
        "3 tao ttt",
        "4 fei fff",
        "Executed.",
        "db >",
    ]
    assert output == expect


@log_func
@db_context_manage
def test_database_persistence_pressure(dbname):
    """数据库存储功能压力测试"""
    commands = [f"insert {i} user{i} email{i}" for i in range(999)]
    commands.append(".exit")
    run_sql_commands(dbname, commands)

    output = run_sql_commands(dbname, ["select", ".exit"])
    print(output[-50:])


@log_func
@db_context_manage
def test_print_constants(dbname):
    """打印所有的常数值"""
    commands = [".constants", ".exit"]
    output = run_sql_commands(dbname, commands)
    print(output)
    expect = ['db > Constants:',
              'ROW_SIZE: 293',
              'COMMON_NODE_HEADER_SIZE: 6',
              'LEAF_NODE_HEADER_SIZE: 14',
              'LEAF_NODE_CELL_SIZE: 297',
              'LEAF_NODE_SPACE_FOR_CELLS: 4082',
              'LEAF_NODE_MAX_CELLS: 13',
              'db >']
    assert output == expect


@log_func
@db_context_manage
def test_print_structure_of_one_node_btree(dbname):
    commands = [f"insert {i} user#{i} person#{i}@example.com" for i in range(14, 0, -1)]
    commands.append(".btree")
    commands.append("insert 15 user15 person15@example.com")
    commands.append(".exit")
    output = run_sql_commands(dbname, commands)
    print(output)
    expect = ['db > Executed.',
              'db > Executed.',
              'db > Executed.',
              'db > Executed.',
              'db > Executed.',
              'db > Executed.',
              'db > Executed.',
              'db > Executed.',
              'db > Executed.',
              'db > Executed.',
              'db > Executed.',
              'db > Executed.',
              'db > Executed.',
              'db > Executed.',
              'db > Tree:',
              '- internal (size 1)',
              '- leaf (size 7)',
              '- 1',
              '- 2',
              '- 3',
              '- 4',
              '- 5',
              '- 6',
              '- 7',
              '- key 7',
              '- leaf (size 7)',
              '- 8',
              '- 9',
              '- 10',
              '- 11',
              '- 12',
              '- 13',
              '- 14',
              'db > Executed.',
              'db >']
    assert output == expect


@log_func
@db_context_manage
def test_print_all_rows_in_a_multi_level_tree(dbname):
    commands = [f"insert {i} user#{i} person#{i}@example.com" for i in range(15, 0, -1)]
    commands.append("select")
    commands.append(".btree")
    commands.append(".exit")
    output = run_sql_commands(dbname, commands)
    print(output)


if __name__ == "__main__":
    file_name = "./db/test.db"
    test_database_operations(file_name)
    # test_database_pressure(file_name)
    test_database_long_string(file_name)
    test_database_too_long_string(file_name)
    test_database_persistence(file_name)
    # test_database_persistence_pressure(file_name)
    test_print_constants(file_name)
    test_print_structure_of_one_node_btree(file_name)
    test_print_all_rows_in_a_multi_level_tree(file_name)
