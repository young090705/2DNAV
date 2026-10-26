from pathlib import Path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import ComposableNodeContainer, Node
from launch_ros.descriptions import ComposableNode
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description():
    share = Path(get_package_share_directory('map_server'))
    return LaunchDescription([
        DeclareLaunchArgument(
            'pcd_file', description='Absolute PCD path, or filename under map_server/maps'),
        DeclareLaunchArgument('frame_id', default_value='map'),
        DeclareLaunchArgument('topic_name', default_value='/map_server/map_cloud'),
        DeclareLaunchArgument('publish_rate', default_value='1.0'),
        DeclareLaunchArgument('use_sim_time', default_value='false'),
        DeclareLaunchArgument('rviz', default_value='false'),
        ComposableNodeContainer(
            name='map_server_container',
            namespace='',
            package='rclcpp_components',
            executable='component_container',
            output='screen',
            composable_node_descriptions=[
                ComposableNode(
                    package='map_server',
                    plugin='map_server::MapServerNode',
                    name='map_server',
                    parameters=[str(share / 'config' / 'params.yaml'), {
                        'pcd_file': ParameterValue(LaunchConfiguration('pcd_file'), value_type=str),
                        'frame_id': ParameterValue(LaunchConfiguration('frame_id'), value_type=str),
                        'topic_name': ParameterValue(LaunchConfiguration('topic_name'), value_type=str),
                        'publish_rate': ParameterValue(
                            LaunchConfiguration('publish_rate'), value_type=float),
                        'use_sim_time': ParameterValue(
                            LaunchConfiguration('use_sim_time'), value_type=bool),
                    }],
                ),
            ],
        )
    ])
