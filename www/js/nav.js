/**
 * Created by Administrator on 2016/10/2.
 */
(function ($) {
    $.fn.extend({
        h_nav : function ( option ) {//defaultOpen默认打开项
            var defOpen = option && option.open || 'li:first-child > a';
            //自定义事件open
            this.on( 'open', 'a', function () {
                var $this = $( this ),
                    $iframe = $( '#main' ),
                    $thisUrl = $this.attr( "href" ),
                    $li = $this.parent( 'li' ),
                    isopen = $li.hasClass( 'open' );//当前选项是否被打开

                if (isopen){ //如果当前选项被打开，则return；
                    return false;
                }else {
                    $li.addClass( 'open' ).siblings( 'li' ).removeClass( 'open' );//当前选项添加open类，并移除当前被打开选项的open类
                    $iframe.attr( "src", $thisUrl ); //更改iframe的src值；切换页面
                }
            } );
            this.find( defOpen ).trigger( 'open' );//触发自定义open事件
            this.on( 'click', 'a', function ( event ) {
                event.preventDefault();//阻止默认行为
                $( this ).trigger( 'open' );//触发自定义open事件
        });
    }
    });
})(jQuery);

