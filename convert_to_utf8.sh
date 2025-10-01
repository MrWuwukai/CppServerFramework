#!/bin/bash

# 设置要处理的目录，默认为当前目录 .
TARGET_DIR=${1:-.}

# 支持的文本文件扩展名，可根据需要增减
TEXT_FILE_EXTENSIONS=("c" "cpp" "h" "hpp" "cc" "hh" "txt" "md" "json" "xml" "yaml" "yml" "ini" "conf" "py" "js" "html" "css" "sh" "log")

# 查找目标目录下所有指定扩展名的文件
find "$TARGET_DIR" -type f | while read -r file; do
    # 获取文件扩展名
    ext="${file##*.}"
    
    # 检查扩展名是否在目标列表中
    is_text_file=0
    for allowed_ext in "${TEXT_FILE_EXTENSIONS[@]}"; do
        if [[ "$ext" == "$allowed_ext" ]]; then
            is_text_file=1
            break
        fi
    done

    if [[ $is_text_file -eq 0 ]]; then
        continue
    fi

    echo "检查文件: $file"

    # 使用 enca 检测编码
    encoding=$(enca -L zh_CN -i "$file" 2>/dev/null)

    # 判断编码是否是 UTF-8（忽略大小写与无 BOM 问题）
    if [[ "$encoding" == *"utf-8"* ]] || [[ "$encoding" == "utf8" ]]; then
        echo "  → 已是 UTF-8，跳过: $file"
    else
        echo "  → 检测到编码: $encoding ，尝试转换为 UTF-8 ..."

        # 备份原文件（可选，如需备份可取消下一行注释）
        # cp "$file" "${file}.bak"

        # 转换编码为 UTF-8（使用 iconv）
        # 注意：enca 可能返回 GBK、GB2312、ISO-8859-1 等
        # 我们统一用 iconv 从检测到的编码转 UTF-8
        if enca -L zh_CN -i "$file" >/dev/null 2>&1; then
            detected_encoding=$(enca -L zh_CN -i "$file")
            echo "  → 原始编码: $detected_encoding"

            # 尝试转换（iconv 需要明确的 from-encoding）
            case "$detected_encoding" in
                *GBK*|*gbk*)
                    from_enc="GBK"
                    ;;
                *GB2312*|*gb2312*)
                    from_enc="GB2312"
                    ;;
                *ISO-8859-1*|*iso-8859-1*|*latin1*)
                    from_enc="ISO-8859-1"
                    ;;
                *)
                    echo "  → 无法识别的编码 '$detected_encoding'，跳过转换"
                    continue
                    ;;
            esac

            # 执行转换
            iconv -f "$from_enc" -t UTF-8 "$file" -o "$file.tmp" && \
            mv "$file.tmp" "$file" && \
            echo "  → ✅ 已转换为 UTF-8: $file"
        else
            echo "  → ⚠️ 无法检测文件编码，跳过: $file"
        fi
    fi
done

echo "转换完成！"