#!/bin/bash
set -euo pipefail

# ====================== 配置项（请修改这里！）======================
# 1. 本地Git仓库绝对路径（必填，不是远程URL）
GIT_REPO_PATH="/path/to/your/git/repo"
# 2. 要修改的分支名（必填，如 main/master/dev）
TARGET_BRANCH="master"
# 3. 旧的用户名（留空表示修改所有提交）
OLD_USER_NAME=""
# 4. 旧的邮箱（留空表示修改所有提交）
OLD_USER_EMAIL=""
# 5. 新的用户名（必填）
NEW_USER_NAME=""
# 6. 新的邮箱（必填）
NEW_USER_EMAIL=""
# ==================================================================

# 颜色输出（方便查看日志）
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # 无颜色

# 检查依赖：git-filter-repo
check_dependency() {
    if ! command -v git-filter-repo &> /dev/null; then
        echo -e "${RED}错误：未安装 git-filter-repo${NC}"
        echo -e "${YELLOW}安装命令：${NC}"
        echo "  macOS: brew install git-filter-repo"
        echo "  Ubuntu/Debian: apt install git-filter-repo"
        echo "  pip: pip install git-filter-repo"
        exit 1
    fi
}

# 校验配置参数
validate_config() {
    # 校验仓库路径
    if [ ! -d "$GIT_REPO_PATH/.git" ]; then
        echo -e "${RED}错误：Git 仓库路径不存在或不是合法仓库：$GIT_REPO_PATH${NC}"
        exit 1
    fi

    # 校验必填项
    if [ -z "$NEW_USER_NAME" ] || [ -z "$NEW_USER_EMAIL" ]; then
        echo -e "${RED}错误：NEW_USER_NAME 和 NEW_USER_EMAIL 不能为空${NC}"
        exit 1
    fi

    # 校验分支存在
    cd "$GIT_REPO_PATH" || exit 1
    if ! git show-ref --verify --quiet "refs/heads/$TARGET_BRANCH"; then
        echo -e "${RED}错误：分支 $TARGET_BRANCH 不存在${NC}"
        exit 1
    fi
    cd - > /dev/null
}

# 构建 git filter-repo 的 Python callback 代码
build_commit_callback() {
    local callback=""
    callback+="old_name = b\"$OLD_USER_NAME\"\n"
    callback+="old_email = b\"$OLD_USER_EMAIL\"\n"
    callback+="new_name = b\"$NEW_USER_NAME\"\n"
    callback+="new_email = b\"$NEW_USER_EMAIL\"\n"
    callback+="\n"
    callback+="# 匹配条件：旧用户名/邮箱为空时，修改所有提交\n"
    callback+="if not old_name or commit.committer_name == old_name:\n"
    callback+="    if not old_email or commit.committer_email == old_email:\n"
    callback+="        commit.committer_name = new_name\n"
    callback+="        commit.committer_email = new_email\n"
    callback+="\n"
    callback+="if not old_name or commit.author_name == old_name:\n"
    callback+="    if not old_email or commit.author_email == old_email:\n"
    callback+="        commit.author_name = new_name\n"
    callback+="        commit.author_email = new_email\n"
    echo -e "$callback"
}

# 主执行函数
main() {
    echo -e "${YELLOW}==================== 开始执行 ====================${NC}"
    echo -e "仓库路径：${GREEN}$GIT_REPO_PATH${NC}"
    echo -e "目标分支：${GREEN}$TARGET_BRANCH${NC}"
    echo -e "旧用户名：${YELLOW}${OLD_USER_NAME:-[所有用户]}${NC}"
    echo -e "旧邮箱：${YELLOW}${OLD_USER_EMAIL:-[所有邮箱]}${NC}"
    echo -e "新用户名：${GREEN}$NEW_USER_NAME${NC}"
    echo -e "新邮箱：${GREEN}$NEW_USER_EMAIL${NC}"
    echo -e "${RED}⚠️  警告：此操作会重写 Git 提交历史！${NC}"
    read -p "是否继续？(y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo -e "${YELLOW}操作已取消${NC}"
        exit 0
    fi

    # 检查依赖
    check_dependency
    # 校验配置
    validate_config
    # 构建 Python callback 代码
    commit_callback=$(build_commit_callback)
    echo -e "${GREEN}已生成 commit-callback 代码${NC}"

    # 进入仓库执行修改
    cd "$GIT_REPO_PATH" || exit 1
    echo -e "${YELLOW}开始重写提交历史...${NC}"
    
    # 核心命令：使用 git filter-repo 的 --commit-callback（接受 Python 代码）
    git filter-repo \
        --commit-callback "$commit_callback" \
        --refs "$TARGET_BRANCH" \
        --force

    echo -e "${GREEN}==================== 执行完成 ====================${NC}"
    echo -e "✅ 分支 $TARGET_BRANCH 所有提交已修改为："
    echo -e "   用户名：$NEW_USER_NAME"
    echo -e "   邮箱：$NEW_USER_EMAIL"
    echo -e "${YELLOW}提示：如需推送到远程，执行：git push -f origin $TARGET_BRANCH${NC}"
}

# 运行主函数
main
