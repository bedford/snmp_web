(function($){
    $.fn.extend({
        tableExport: function(options) {
            var defaults = {
                    separator: ',',
                    ignoreColumn: [],
                    tableName:'yourTableName',
                    escape:'true',
                    htmlContent:'false',
                    fileName: 'data_record',
            };

            var options = $.extend(defaults, options);
            var el = this;

            var excel="<table>";
            // Header
            $(el).find('thead').find('tr').each(function() {
                excel += "<tr>";
                $(this).filter(':visible').find('th').each(function(index,data) {
                    if ($(this).css('display') != 'none') {
                        if(defaults.ignoreColumn.indexOf(index) == -1) {
                            excel += "<td>'" +parseString($(this))+ "</td>";
                        }
                    }
                });
                excel += '</tr>';
            });					
            
            // Row Vs Column
            var rowCount=1;
            $(el).find('tbody').find('tr').each(function() {
                excel += "<tr>";
                var colCount=0;
                $(this).filter(':visible').find('td').each(function(index,data) {
                    if ($(this).css('display') != 'none'){	
                        if(defaults.ignoreColumn.indexOf(index) == -1){
                            excel += "<td>'"+parseString($(this))+"</td>";
                        }
                    }
                    colCount++;
                });															
                rowCount++;
                excel += '</tr>';
            });					
            excel += '</table>'

            var excelFile = "<html xmlns:o='urn:schemas-microsoft-com:office:office' xmlns:x='urn:schemas-microsoft-com:office:"+defaults.type+"' xmlns='http://www.w3.org/TR/REC-html40'>";
            excelFile += "<head>";
            excelFile += "<!--[if gte mso 9]>";
            excelFile += "<xml>";
            excelFile += "<x:ExcelWorkbook>";
            excelFile += "<x:ExcelWorksheets>";
            excelFile += "<x:ExcelWorksheet>";
            excelFile += "<x:Name>";
            excelFile += "工作表1";
            excelFile += "</x:Name>";
            excelFile += "<x:WorksheetOptions>";
            excelFile += "<x:DisplayGridlines/>";
            excelFile += "</x:WorksheetOptions>";
            excelFile += "</x:ExcelWorksheet>";
            excelFile += "</x:ExcelWorksheets>";
            excelFile += "</x:ExcelWorkbook>";
            excelFile += "</xml>";
            excelFile += "<![endif]-->";
            excelFile += "</head>";
            excelFile += "<body>";
            excelFile += excel;
            excelFile += "</body>";
            excelFile += "</html>";

            /*var base64data = "base64," + $.base64.encode(excelFile);
            window.open('data:application/vnd.ms-'+defaults.type+';filename=exportData.doc;' + base64data);*/
            var uri = 'data:application/vnd.ms-excel;charset=utf-8,' + encodeURIComponent(excelFile);
            var link = document.createElement("a");
            link.href = uri;
        
            link.style = "visibility:hidden";
            link.download = defaults.fileName + ".xls";

            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);

            function parseString(data) {
                if (defaults.htmlContent == 'true') {
                    content_data = data.html().trim();
                } else {
                    content_data = data.text().trim();
                }

                if (defaults.escape == 'true') {
                    content_data = escape(content_data);
                }

                return content_data;
            }

        }
    });
})(jQuery);