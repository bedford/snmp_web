function load_email_param() {
	var param_list = [
	{
		"id":"100",
		"name":"张三",
		"email":"13828891024@qq.com"
	},
	{
		"id":"101",
		"name":"李四",
		"email":"13828891024@139.com"
	}
	];
	show_email_list(param_list);
}

function show_email_list(email_param_list) {
	var html="";
	$.each(email_param_list, function(i, item) {
		html+="<tr onclick='onClick(this)' id='"+item.id+"'>";
		html+="<td>"+(i+1)+"</td>"+"<td>"+item.name+"</div></td>";
		html+="<td>"+item.email+"</td>"; 
		html+='<td><a href="#" onclick="edit_item(\''+item.id+'\')">编辑</a> <a href="#" onclick="delete_item(\''+item.id+'\')">删除</a></td>';
		html+="</tr>"
	});
	$("#sms_table").append(html);
}

function load_sms_param() {
	var param_list = [
	{
		"id":"100",
		"name":"张三",
		"phone":"13828891024"
	},
	{
		"id":"101",
		"name":"李四",
		"phone":"13828891024"
	}
	];
	show_sms_list(param_list);
}

function show_sms_list(sms_param_list) {
	var html="";
	$.each(sms_param_list, function(i, item) {
		html+="<tr onclick='onClick(this)' id='"+item.id+"'>";
		html+="<td>"+(i+1)+"</td>"+"<td>"+item.name+"</td>";
		html+="<td>"+item.phone+"</td>"; 
		html+='<td><a href="#" onclick="edit_item(\''+item.id+'\')">编辑</a> <a href="#" onclick="delete_item(\''+item.id+'\')">删除</a></td>';
		html+="</tr>"
	});
	$("#sms_table").append(html);
}

function delete_item(index) {
	//利用对话框返回的值(true 或者 false)
    if (confirm("你确定删除这条记录吗?")) {
        alert("点击了确定");
    }
    else {
        alert("点击了取消");  
    }
}

function edit_item(index) {
	layer.open({
		type: 1,
		area: ['600px', '360px'],
		shadeClose: true,
		content: '\<\div style="padding:20px;">自定义内容\<\/div>'
	});
}