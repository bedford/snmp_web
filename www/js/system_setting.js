/* 加载系统参数 */
function load_system_param() {
    var json = new Object();
    json.msg_type = 3;
    json.cmd_type = 0;
    var data = $.toJSON(json);
    $.ajax({
                url     : "/cgi-bin/common.cgi",
                type    : "POST",
                dataType: "json",
                data    : data,
                success : function(msg) {
                	$("#site").val(msg.site);
					$("#device_number").val(msg.device_number);
                },
            });
}

function set_system_param() {
	var json = new Object();
	json.msg_type = 3;
	json.cmd_type = 1;

	var cfg = new Object();
	cfg.site = $("#site").val();
	cfg.device_number = $("#device_number").val();
	json.cfg = cfg;

	$('#set_system_btn').attr("disabled", "disabled");
	var data = $.toJSON(json);
    $.ajax({
                url     : "/cgi-bin/common.cgi",
                type    : "POST",
                dataType: "json",
                data    : data,
                success : function(msg) {
                    $('#set_system_btn').removeAttr("disabled");
					if (msg.status == 1) {
						alert("设置成功");
					} else {
						alert("设置失败");
					}
                },
			});
}

function reboot() {
	var json = new Object();
	json.msg_type = 3;
	json.cmd_type = 2;

	$('#reboot_btn').attr("disabled", "disabled");
	var data = $.toJSON(json);
    $.ajax({
                url     : "/cgi-bin/common.cgi",
                type    : "POST",
                dataType: "json",
                data    : data,
                success : function(msg) {
                    $('#reboot_btn').removeAttr("disabled");
					if (msg.status == 1) {
						alert("设置成功");
					} else {
						alert("设置失败");
					}
                },
			});
}

function recovery() {
	var json = new Object();
	json.msg_type = 3;
	json.cmd_type = 3;

	$('#recovery_btn').attr("disabled", "disabled");
	var data = $.toJSON(json);
    $.ajax({
                url     : "/cgi-bin/common.cgi",
                type    : "POST",
                dataType: "json",
                data    : data,
                success : function(msg) {
                    $('#recovery_btn').removeAttr("disabled");
					if (msg.status == 1) {
						alert("设置成功");
					} else {
						alert("设置失败");
					}
                },
			});
}

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

function update_firmware() {
	var filename = $('#update_file').val();
	var form_data = new FormData($('#update_form')[0]);

    $.ajax({
                url     : "/cgi-bin/update.cgi",
                type    : "POST",
                dataType: "json",
                data    : form_data,
				cache	: false,
				contentType: false,
				processData: false,
                success : function(msg) {
                    alert(msg);
                },
            });
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
