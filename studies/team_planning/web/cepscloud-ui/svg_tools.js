function set_svg_transform_attr(transform_attr,what,param){
    let a = [];
    let last_pos = 0;
    for(let i = 0;i < transform_attr.length;++i){
        if (transform_attr.charAt(i) == ")"){
            a.push(transform_attr.substring(last_pos,i+1));
            let j = i+1;
            for(;j != transform_attr.length && transform_attr.charAt(j) == " ";++j);
            last_pos = j;
            i = last_pos;
        }
    }
    let found = false;
    for(let i = 0; i != a.length;++i){
        if (!a[i].startsWith(what)) continue;
        a[i] = what+param;
        found = true;
    }
    if (!found) a.push(what+param);
    let t = "";
    for(let i = 0; i != a.length-1;++i)
    {
        t = t + a[i] + " ";
    }
    t = t + a[a.length-1];
    return t;
}

function get_svg_transform_attr(transform_attr,what){
    let a = [];
    let last_pos = 0;
    for(let i = 0;i < transform_attr.length;++i){
        if (transform_attr.charAt(i) == ")"){
            a.push(transform_attr.substring(last_pos,i+1));
            let j = i+1;
            for(;j != transform_attr.length && transform_attr.charAt(j) == " ";++j);
            last_pos = j;
            i = last_pos;
        }
    }
    for(let i = 0; i != a.length;++i){
        if (!a[i].startsWith(what)) continue;
        return a[i];
    }
    return undefined;
}

function d3_util_scale(g,scale_x,scale_y){
    let transform_attr = g.attr("transform");
    let new_transform_attr = set_svg_transform_attr(transform_attr,"scale",`(${scale_x} ${scale_y})`);
    g.attr("transform",new_transform_attr);
}

function d3_util_translate_rel(g,d_x_rel,d_y_rel){
    let transform_attr = g.attr("transform");
    let translation_attr = get_svg_transform_attr(transform_attr,"translate");
    console.log(translation_attr);
    //g.attr("transform",new_transform_attr);
}
