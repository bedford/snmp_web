function load_system_info() {
	var param_list = [
	{
		"name":"设备编号",
		"value":"20160820010001"
	},
	{
		"name":"设备安装地点",
		"value":"广东省深圳市大数据机房"
	},
	{
		"name":"设备当前时间",
		"value":"2016-12-24 17:00:00"
	},
	{
		"name":"设备运行时长",
		"value":"300分钟"
	},
	{
		"name":"硬件版本",
		"value":"V1.2"
	},
	{
		"name":"BSP版本",
		"value":"Kernel-2.6.31-20160110"
	},
	{
		"name":"软件版本",
		"value":"V0.3"
	},
	{
		"name":"设备序列号",
		"value":"20160820010001"
	}
	];
	show_system_info(param_list);
}

function show_system_info(system_param_list) {
	var html="";
	$.each(system_param_list, function(i, item) {
		html+="<tr><td>"+item.name+"</td>";
		html+="<td>"+item.value+"</td>";
		html+="</tr>"
	});
	$("#system_info_table").append(html);
}


function load_user() {
	var user_list = [
	{
		"id":"100",
		"user":"admin",
		"password":"*****",
		"authority":"管理员"
	},
	{
		"id":"101",
		"user":"monitor",
		"password":"*****",
		"authority":"监查员"
	},
	{
		"id":"102",
		"user":"operator",
		"password":"*****",
		"authority":"操作员"
	},
	{
		"id":"103",
		"user":"guest",
		"password":"*****",
		"authority":"访客"
	}
	];
	show_user_list(user_list);
}

function show_user_list(user_list) {
	var html="";
	$.each(user_list, function(i, item) {
		html+="<tr onclick='onClick(this)' id='"+item.id+"'>";
		html+="<td>"+(i+1)+"</td>"+"<td>"+item.user+"</div></td>";
		html+="<td>"+item.password+"</td>";
		html+="<td>"+item.authority+"</td>"; 
		html+='<td><a href="#" onclick="edit_item(\''+item.id+'\')">编辑</a> <a href="#" onclick="delete_item(\''+item.id+'\')">删除</a></td>';
		html+="</tr>"
	});
	$("#user_table").append(html);
}