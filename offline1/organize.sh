#!/bin/bash
if [ $# -lt 4 ];then
    echo "Usage"
    echo "./organize.sh <submission_folder> <target_folder> <test_folder> <answerfolder> [-v] [-noexecute]"
    echo "-v: verbose"
    echo "-noexecute: do not execute code files"
    kill -INT $$
fi
directory=$1
to="mymatch/"
temp="mymatch/temp/"
tests=$3
ans=$4
arg1=$5
arg2=$6
noOfTests=$(ls "$3" | wc -l)
numberOfAns=$(ls "$ans" | wc -l)

if [ "$5" = -v ] || [ "$6" = -v ];then
    echo "Found $noOfTests test files"
fi

search(){
    dir=$1
    id=$2
    for file in "$dir"/*
    do
        if [ -f "$file" ];then
            ext=${file#*.}
            if [ $ext = py ] || [ $ext = c ] || [ $ext = java ];then
                if [ "$arg1" = -v ] || [ "$arg2" = -v ];then
                    echo "Organizing files of $2"
                fi
                lang=$(changeExt "$ext")
                mkdir -p "$target/$lang/$id/"
                cp  "$file" "$target/$lang/$id/main.$ext"
                if [ "$arg1" = -noexecute ] || [ "$arg2" = -noexecute ];then
                    continue
                fi
                if [ "$arg1" = -v ] || [ "$arg2" = -v ];then
                    echo "Executing files of $2"
                fi
                if [ $ext = py ];then
                    runPy "$target/$lang/$id/main.$ext" "$target/$lang/$id/"
                elif [ $ext = c ];then
                    runC "$target/$lang/$id/main.$ext" "$target/$lang/$id/"
                elif [ $ext = java ];then
                    runJava "$target/$lang/$id/main.$ext" "$target/$lang/$id/"
                fi
                a=$(checkAns "$target/$lang/$id/" $lang $id)
                echo "$a" >> "$target/result.csv"
            fi
        elif [ -d "$file" ];then
            search "$file" "$id"
        fi
    done
}

changeExt(){
    ext=$1
    if [ $ext = py ];then
        echo "Python"
    elif [ $ext = c ];then
        echo "C"
    elif [ $ext = java ];then
        echo "Java"
    fi
}

checkAns(){
    path=$1
    ext=$2
    id=$3
    nRight=0
    nWrong=0
    for((i=1;i<=numberOfAns;i++));do
        difference=$(diff "$path/out$i.txt" "$ans/ans$i.txt")
        if [ "$difference" = "" ];then
            nRight=`expr $nRight + 1`
        else
            nWrong=`expr $nWrong + 1`
        fi
    done

    echo "$id,$ext,$nRight,$nWrong"
}

runC(){
    file=$1
    path=$2
    gcc "$file" -o "$path/main"
    i=1
    for test in "$tests"/*;do
        "$path/main" < "$test" > "$path/out$i.txt"
        i=`expr $i + 1`
    done
}

runPy(){
    file=$1
    path=$2
    i=1
    for test in "$tests"/*;do
        python3 "$file" < "$test" > "$path/out$i.txt"
        i=`expr $i + 1`
    done
    
}

runJava(){
    file=$1
    path=$2
    javac "$file"
    filename=$(find "$path" -name "*.class")
    
    
    # filename is like this this/that/CLASSNAME.class. we need to extract CLASSNAME from it. classname can be different length
    # so we need to find the last / and then extract the string after it
    className=${filename##*/}
    classNameWithoutExt=${className%.*}

    i=1
    for test in "$tests"/*;do
        java -cp "$path" "$classNameWithoutExt" < "$test" > "$path/out$i.txt"
        i=`expr $i + 1`
    done
}

mkdir -p $temp

target=$2
rm -r "$target"
mkdir "$target"

if [ "$arg1" != -noexecute ] && [ "$arg2" != -noexecute ];then
    echo "student_id,type,matched,not_matched" > "$target/result.csv"
fi
# finding all the files in the directory
for file in $directory/*
do
    # echo "$file"
    # extracting the last 7 characters
    id=${file: -11:-4}
    # unzip the file and send it to temp folder with the id
    unzip -qq "$file" -d "$temp$id"
    
    # find the source code 
    search "$temp$id" "$id"

done

rm -r "$to"