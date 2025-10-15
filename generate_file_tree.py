#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
文件结构生成器 - 修复版
修复内容：添加Windows系统大小写不敏感支持，优化路径处理
"""

import os
import sys
import argparse
from pathlib import Path
from typing import Set, List

def generate_file_tree(
    root_path: str, 
    exclude_dirs: Set[str] = None,
    max_depth: int = None
) -> List[str]:
    """
    生成目录树结构
    
    Args:
        root_path: 根目录路径
        exclude_dirs: 需要排除的目录集合
        max_depth: 最大递归深度
    
    Returns:
        格式化行列表
    """
    if exclude_dirs is None:
        exclude_dirs = set()
    
    # 转换为绝对路径并规范化（处理Windows路径分隔符和大小写）
    root = Path(root_path).resolve()
    if not root.exists():
        raise FileNotFoundError(f"目录不存在: {root_path}")
    
    # 处理Windows大小写不敏感问题：将排除目录名转为小写
    exclude_dirs_lower = {d.lower() for d in exclude_dirs}
    result_lines = []
    
    def walk_directory(current_path: Path, prefix: str = "", depth: int = 0):
        """递归遍历目录"""
        if max_depth and depth > max_depth:
            return
            
        try:
            # 获取所有子项并排序（目录优先，按名称排序）
            items = list(current_path.iterdir())
            directories = sorted([i for i in items if i.is_dir()])
            files = sorted([i for i in items if i.is_file()])
            
            # 合并目录和文件
            all_items = directories + files
            
            if not all_items:
                return
                
            for index, item in enumerate(all_items):
                is_last = (index == len(all_items) - 1)
                connector = "└─ " if is_last else "├─ "
                
                # 检查是否需要排除（修复：添加大小写不敏感比较）
                if item.is_dir():
                    # Windows系统大小写不敏感处理
                    if sys.platform.startswith('win'):
                        if item.name.lower() in exclude_dirs_lower:
                            continue
                    else:
                        if item.name in exclude_dirs:
                            continue
                    
                # 构建当前行
                current_prefix = prefix + ("│  " if not is_last else "   ")
                line = f"{prefix}{connector}{item.name}"
                
                # 添加注释说明（如果是常见文件）
                comment = get_file_comment(item.name)
                if comment:
                    line = f"{line:<50} # {comment}"
                    
                result_lines.append(line)
                
                # 递归处理子目录
                if item.is_dir():
                    walk_directory(item, current_prefix, depth + 1)
                    
        except PermissionError:
            result_lines.append(f"{prefix}├─ [权限不足，无法访问]")
        except Exception as e:
            result_lines.append(f"{prefix}├─ [错误: {str(e)}]")
    
    # 添加根目录行
    result_lines.append(f"{root.name}/")
    walk_directory(root)
    
    return result_lines

def get_file_comment(filename: str) -> str:
    """获取文件的注释说明"""
    comments = {
        # 项目配置
        'app.py': '程序入口：加载样式/DB/主窗体，注册插件与服务',
        'config.py': '全局配置（路径、样式、默认参数、日志级别等）',
        'requirements.txt': '仅保留最小依赖：PyQt5、networkx、numpy',
        'README.md': '',
        '.gitignore': '',
        
        # 数据库
        'interface.sqlite3': '运行期生成/替换（可放到 data/ 下）',
        'schema.sql': '首版建表脚本（见下“核心表”）',
        
        # Python模块
        '__init__.py': '',
        'app_context.py': '全局服务定位/依赖注入（db、repo、services）',
        'event_bus.py': '轻量事件与订阅（UI/服务间解耦）',
        'plugin_loader.py': '模块/接口/环境“建模代码”的安全装载',
        'safe_exec.py': '受限 exec 执行沙箱（限制内建、注入上下文）',
        'utils.py': '通用工具（ID、时间、校验、图算法薄封装等）',
        'db.py': '连接、事务、初始化（schema.sql）',
        'models.py': 'dataclass：Module/Port/Interface/Connection/…',
        'faults.py': '失效模式、触发条件（结构定义）',
        'task_profile.py': '任务剖面/成功判据模型',
        'environment.py': '环境应力/参数模型（可与接口概率发生关联）',
        'constants.py': '接口五大类、Gate类型、单位/维度等枚举',
        'modeling_service.py': '模块库/接口库装载与实例化、画布模型转换',
        'graph_service.py': '系统结构图（模块-端口-连线）的有向图维护',
        'simulate_service.py': '给定环境&剖面，评估接口触发概率/状态（支持蒙特卡洛）',
        'fta_builder.py': '从系统图+任务判据自动生成故障树（AND/OR/基本事件）',
        'fta_qualitative.py': '定性分析：最小割集/单点故障识别',
        'fta_quantitative.py': '定量分析：顶事件概率、重要度、敏感性',
        'report_service.py': '图表与结果打包（导出JSON/HTML/PNG）',
        'io_service.py': '项目导入导出（.json）与样例数据装载',
        
        # UI相关
        'MainWindow.ui': '主框架：左-库/中-画布/右-属性与结果Dock',
        'ModuleLibraryPanel.ui': '模块/接口/环境库（拖拽到画布）',
        'CanvasEditor.ui': '类似 Simulink 的图形建模画布（QGraphicsView）',
        'PropertyInspector.ui': '选中对象的属性编辑（端口、变量、参数、图标）',
        'TaskProfileEditor.ui': '任务剖面/成功判据（可选目标输出绑定）',
        'EnvironmentEditor.ui': '环境模块与参数（风浪/温度/负载等）',
        'FaultTreeView.ui': '故障树可视化（门/事件节点）',
        'AnalysisResultView.ui': '定性/定量结果的图表与表格',
        'ProjectSelector.ui': '打开/新建项目、样例列表',
        'styles.qss': '简洁主题样式',
        
        # 组件
        'graph_canvas.py': '画布控件：节点/端口/连线/对齐/吸附/撤销栈',
        'node_items.py': '模块节点（带图标与接口连接点）',
        'ftree_widget.py': '故障树绘制控件（支持折叠与导出PNG/SVG）',
        'editors.py': '端口变量编辑、Python代码编辑(语法高亮)',
        
        # 其他
        'resources.qrc': '可选（若用 rcc），但首版建议直接文件路径',
    }
    return comments.get(filename, '')

def main():
    parser = argparse.ArgumentParser(
        description='生成目录树结构',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
使用示例:
  %(prog)s .                          # 生成当前目录结构
  %(prog)s /path/to/project           # 生成指定目录结构
  %(prog)s . --exclude tests docs     # 排除tests和docs目录
  %(prog)s . --exclude node_modules   # 排除node_modules目录
        """
    )
    
    parser.add_argument(
        'path',
        nargs='?',
        default='.',
        help='目标目录路径 (默认: 当前目录)'
    )
    
    parser.add_argument(
        '--exclude', '-e',
        nargs='*',
        default=[],
        help='要排除的目录名（自动跳过该目录及其子目录）'
    )
    
    parser.add_argument(
        '--max-depth', '-d',
        type=int,
        help='最大递归深度'
    )
    
    args = parser.parse_args()
    
    try:
        # 生成文件树
        lines = generate_file_tree(
            args.path, 
            set(args.exclude),
            args.max_depth
        )
        
        # 输出结果
        for line in lines:
            print(line)
            
    except Exception as e:
        print(f"❌ 错误: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()